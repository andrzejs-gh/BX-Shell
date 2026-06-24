#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/uart.h"

#include "../input_line.h"
#include "../CONSTANTS.h"
#include "../GLOBAL_VARIABLES.h"

int hello(int argc, char** argv)
{
    printf(NEW_LINE "Hello! You passed:" NEW_LINE);

    for (int i = 0; i < argc; i++)
        printf("argv[%d] = %s" NEW_LINE, i, argv[i]);

    printf(NEW_LINE "Feel free to use, fork and extend BX Shell however you want!" NEW_LINE);

    return 0;
}

int input_line_test(int argc, char** argv)
{
    input_line test_line = new_input_line(32, ECHO);
    char* addr = (char*)test_line.buffer.addr;

    char* title = NEW_LINE "=== Input line test ===" NEW_LINE "(Press Ctrl+C to quit)" NEW_LINE NEW_LINE;
    uart_write_bytes(UART_NUM_0, title, strlen(title));

    char* normal_input = "Normal input: ";
    uart_write_bytes(UART_NUM_0, normal_input, strlen(normal_input));
    char* failed_input_read = "Failed input read." NEW_LINE "Continuing..." NEW_LINE;
    char* entered_text = "Entered text: %s" NEW_LINE NEW_LINE;
    int ret = read_input(&test_line);
    if (ret == CTRL_C_PRESSED)
    {
        uart_write_bytes(UART_NUM_0, NEW_LINE, NEW_LINE_LEN);
        return CTRL_C_PRESSED;
    }
    if (ret == -1)
        uart_write_bytes(UART_NUM_0, failed_input_read, strlen(failed_input_read));
    else
        printf(entered_text, addr);
    reset_input_line(&test_line);

    test_line.echo_mode = MASK;
    char* masked_input = "Masked input: ";
    uart_write_bytes(UART_NUM_0, masked_input, strlen(masked_input));
    ret = read_input(&test_line);
    if (ret == CTRL_C_PRESSED)
    {
        uart_write_bytes(UART_NUM_0, NEW_LINE, NEW_LINE_LEN);
        return CTRL_C_PRESSED;
    }
    if (ret == -1)
        uart_write_bytes(UART_NUM_0, failed_input_read, strlen(failed_input_read));
    else
        printf(entered_text, addr);
    reset_input_line(&test_line);

    test_line.echo_mode = SUPPRESS;
    char* suppressed_input = "Suppressed input: ";
    uart_write_bytes(UART_NUM_0, suppressed_input, strlen(suppressed_input));
    ret = read_input(&test_line);
    if (ret == CTRL_C_PRESSED)
    {
        uart_write_bytes(UART_NUM_0, NEW_LINE, NEW_LINE_LEN);
        return CTRL_C_PRESSED;
    }
    if (ret == -1)
        uart_write_bytes(UART_NUM_0, failed_input_read, strlen(failed_input_read));
    else
        printf(entered_text, addr);
    reset_input_line(&test_line);

    free_input_line(&test_line);

    return 0;
}

int hist(int argc, char** argv)
{
    char** cmd_line_history = cmd_line.history;

    for (int i = 0; i < HISTORY_CAPACITY; i++)
        printf("%d. %s" NEW_LINE, i, cmd_line_history[i]);

    return 0;
}

int cmd_line_set_restr_mode(int argc, char** argv)
{
    if (argc == 1)
    {
        printf("No argument passed. You must type either \"on\" or \"off\"." NEW_LINE);
        return -1;
    }
    else if (argc > 2)
    {
        printf("Too many arguments. You must type either \"on\" or \"off\"." NEW_LINE);
        return 1;
    }

    if (!strcmp(argv[1], "on"))
    {
        cmd_line.is_restricted = 1;
        printf("Command line is now in RESTRICTED mode." NEW_LINE);
    }
    else if (!strcmp(argv[1], "off"))
    {
        cmd_line.is_restricted = 0;
        printf("Command line is now in UN-RESTRICTED mode." NEW_LINE 
			   "**WARNING**: Correct input handling and shell stability " 
			   "IS NOT guaranteed in unrestricted mode." NEW_LINE);
    }
    else
    {
        printf("Invalid argument: \"%s\". " 
			   "You must type either \"on\" or \"off\"." NEW_LINE, argv[1]);
        return 2;
    }

    return 0;
}

int cmd_line_set_echo_mode(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("No mode specified. You must pass:" NEW_LINE 
			   "-e - normal output" NEW_LINE
			   "-m - output masked with \"*\"" NEW_LINE
			   "-s - invisible output" NEW_LINE);
		return 1;
	}
	
	char* arg = argv[1];
	
	if (!strcmp(arg, "-e"))
	{
		cmd_line.echo_mode = ECHO;
		printf("Echo mode set to ECHO" NEW_LINE);
	}
	else if (!strcmp(arg, "-m"))
	{
		cmd_line.echo_mode = MASK;
		printf("Echo mode set to MASK" NEW_LINE);
	}
	else if (!strcmp(arg, "-s"))
	{
		cmd_line.echo_mode = SUPPRESS;
		printf("Echo mode set to SUPPRESS" NEW_LINE);
	}
	else
	{
		printf("Invalid argument: \"%s\". You must pass:" NEW_LINE
			   "-e - normal output" NEW_LINE
			   "-m - output masked with \"*\"" NEW_LINE
			   "-s - invisible output" NEW_LINE,
			   arg);
		return 2;
	}
	
	return 0;
}
