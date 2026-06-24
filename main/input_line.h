#ifndef INPUT_LINE_H
#define INPUT_LINE_H

#include <stdint.h>
#include "cont.h"

typedef struct
{
	cont buffer;
	uint8_t cursor;
	uint8_t viewport_begin;
	uint16_t first_column;
	char echo_mode;
	uint8_t is_restricted;
	char** history;
	uint8_t history_index;
	
} input_line;

input_line new_input_line(uint8_t buffer_size, char echo_mode);
int free_input_line(input_line* inpl);
int reset_input_line(input_line* inpl);
int read_input(input_line* inpl);
uint8_t update_history(input_line* inpl);

#endif
