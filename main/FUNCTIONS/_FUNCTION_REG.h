#include <stdio.h>
#include "../function_table.h"
#include "../CONSTANTS.h"
#include "../GLOBAL_VARIABLES.h"

#include "built-ins.h"

void register_commands(void)
{
	ftable_entry FUNCTION_REG[] = {
			{"hello", hello}, 
			{"test", input_line_test},
			{"hist", hist}, 
			{"restr", cmd_line_set_restr_mode},
			{"echom", cmd_line_set_echo_mode},
			// *** user entries *** 

			
			// *** ============ ***
			{0, 0}
	};
	
	commands = ftable_new(sizeof(FUNCTION_REG) - 1);
	ftable* ft = &commands;
	size_t i = 0;
	
	while (FUNCTION_REG[i].key)
	{
		ftable_set(ft, FUNCTION_REG[i]);
		printf("Registered command: %s" NEW_LINE, FUNCTION_REG[i].key);
		i++;
	}
}
