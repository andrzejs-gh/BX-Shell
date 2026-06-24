#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "driver/uart.h"

#include "bx_shell.h"
#include "input_line.h"
#include "function_table.h"
#include "cont.h"
#include "CONSTANTS.h"

#include "FUNCTIONS/_FUNCTION_REG.h"

input_line cmd_line;			// global variables
ftable commands;				// accessible via:
void* fs_cwd = NULL;			// GLOBAL_VARIABLES.h

char* cmd_line_history[HISTORY_CAPACITY];
char history_block[HISTORY_CAPACITY*CMD_LINE_SIZE];

static uint8_t parse(cont* buffer, cont* argv_cont) //
{
	uint8_t argc = 0;
	uint8_t count = buffer->count;
	unsigned char* addr = buffer->addr;
	void* ptr;
	uint8_t index = 0;
	uint8_t seeking_next_ptr = 1;
	
	while (index < count)
	{
		if ( addr[index] == ' ' )
		{
			seeking_next_ptr = 1;
			addr[index] = '\0';
		}
		else if (seeking_next_ptr)
		{
			ptr = &addr[index];
			cont_push(argv_cont, &ptr);
			argc++;
			seeking_next_ptr = 0;
		}
		index++;
	}
	
	return argc;
}

static uint8_t is_ok(void) // sanity check that can be extended
{
	uint8_t ret = 0;
	
	if (CMD_LINE_SIZE > 0 && CMD_LINE_SIZE <= 255)
		ret = 1;
	else
		printf(NEW_LINE "CMD_LINE_SIZE not in the range (0, 255]. Shell start aborted." NEW_LINE);
		
	return ret;
}

void bx_shell(void)
{
	if (!is_ok())
		return;
	
	uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0);
	
	register_commands();
	
	cont argv_cont = cont_new(10, sizeof(char*));
	
	cmd_line = new_input_line(CMD_LINE_SIZE, ECHO);
	
	for (uint8_t i = 0; i < HISTORY_CAPACITY; i++)
		cmd_line_history[i] = &history_block[i*CMD_LINE_SIZE];
	
	cmd_line.history = cmd_line_history;
	
	int ret;
	ftable_entry* entry;
	char* title; 
	
	while ( 1 )
	{
		if (fs_cwd)
			title = fs_cwd;
		else
			title = CMD_LINE_DEFAULT_TITLE;
		
		uart_write_bytes(UART_NUM_0, "[", 1);
		uart_write_bytes(UART_NUM_0, title, strlen(title));
		uart_write_bytes(UART_NUM_0, "]->", 3);
		
		ret = read_input(&cmd_line);
		if (!ret)
			continue;
		if (ret == -1)
		{
			printf("Failed to read input." NEW_LINE "Continuing..." NEW_LINE);
			reset_input_line(&cmd_line);
			continue;
		}
		if (ret == CTRL_C_PRESSED)
		{
			reset_input_line(&cmd_line);
			continue;
		}
		
		if (cmd_line.echo_mode == ECHO)
			update_history(&cmd_line);
		
		parse(&cmd_line.buffer, &argv_cont);
		
		if ( (entry = ftable_get(&commands, (char*)cmd_line.buffer.addr)) )
		{
			ret = entry->function(argv_cont.count, (char**)argv_cont.addr);
			printf(NEW_LINE ">>> Function returned: %d" NEW_LINE, ret);
		}
		else
			printf("Command: \"%s\" is not registered." NEW_LINE, (char*)cmd_line.buffer.addr);
		
		cont_clear(&argv_cont);
		cont_set_capacity(&argv_cont, 10);
		reset_input_line(&cmd_line);
	}
	
	ftable_free(&commands);
	free_input_line(&cmd_line);
	cont_free(&argv_cont);
}
