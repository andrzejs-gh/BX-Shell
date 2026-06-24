#include <stdio.h>
#include <string.h>

#include "driver/uart.h"

#include "input_line.h"
#include "cont.h"
#include "utf8_sanitizer.h"
#include "CONSTANTS.h"

typedef struct
{
	uint8_t byte_count;
	uint8_t column_count;
} redraw_status;

input_line new_input_line(uint8_t buffer_size, char echo_mode)
{
	if (!buffer_size || (echo_mode != ECHO && echo_mode != MASK && echo_mode != SUPPRESS))
		return INVALID_INPUT_LINE;
	
	cont buffer = cont_new(buffer_size, 1);
	if (!buffer.addr) // initialization failed, INVALID_CONT returned
		return INVALID_INPUT_LINE;
	
	buffer.max_capacity = buffer_size;
	
	return (input_line){
		.buffer = buffer,
		.cursor = buffer.count,
		.viewport_begin = 0,
		.first_column = 0,
		.echo_mode = echo_mode,
		.is_restricted = 1,
		.history = NULL,
		.history_index = UINT8_MAX};
}

int free_input_line(input_line* inpl)
{
	if (!inpl->buffer.addr)
		return INPUT_LINE_ALREADY_FREED;
	
	int ret = cont_free(&inpl->buffer);
	if (!ret) // cont successfully freed
		*inpl = INVALID_INPUT_LINE;
	
	return ret;
}

int reset_input_line(input_line* inpl)
{
	if (!inpl->buffer.addr)
		return INPUT_LINE_ALREADY_FREED;
		
	inpl->buffer.count = 0;
	inpl->cursor = 0;
	inpl->viewport_begin = 0;
	inpl->first_column = 0;
	inpl->history_index = UINT8_MAX;
	
	return 0;
}

static int read_byte_sequence(uint8_t* input_buffer)
{
	int ret = uart_read_bytes(UART_NUM_0, &input_buffer[0], 1, portMAX_DELAY);
	if (ret < 0)
		return ret;
	
	uint8_t len = 1;
	uint8_t index = 1;
	
	while ( (ret = uart_read_bytes(UART_NUM_0, &input_buffer[index], 1, 10 / portTICK_PERIOD_MS)) )
	{
		if (ret < 0)
			return ret;
		
		len++;
		if (len == CMD_LINE_SIZE)
			return CMD_LINE_SIZE;
		
		index++;
	}
	
	return len;
}

static inline uint8_t get_char_len_backwards(uint8_t* ptr)
{
	if ( (*ptr >> 7) == 0x00 ) // plain ASCII
		return 1;
	if ( ( *(ptr-1) & 0xE0 ) == 0xC0 ) // 110xxxxx & 1110 0000 = 1100 0000
		return 2;
	if ( ( *(ptr-2) & 0xF0 ) == 0xE0 ) // 1110 xxxx & 1111 0000 = 1110 0000
		return 3;
	if ( ( *(ptr-3) & 0xF8 ) == 0xF0 ) // 1111 0xxx & 1111 1000 = 1111 0000
		return 4;
	else
		return 0;
}

static inline uint8_t get_char_len_forwards(uint8_t* ptr)
{
	if ( (*ptr >> 7) == 0x00 ) // plain ASCII
		return 1;
	if ( ( *ptr & 0xE0 ) == 0xC0 ) // 110xxxxx & 1110 0000 = 1100 0000
		return 2;
	if ( ( *ptr & 0xF0 ) == 0xE0 ) // 1110 xxxx & 1111 0000 = 1110 0000
		return 3;
	if ( ( *ptr & 0xF8 ) == 0xF0 ) // 1111 0xxx & 1111 1000 = 1111 0000
		return 4;
	else
		return 0;
}

static uint16_t get_current_column(void)
{
    uint16_t current_column = 0;
    uint8_t buffer[32] = {0};
    uint8_t i = 0;
    //i++; "\x1b[<row>;<col>R"
    uart_write_bytes(UART_NUM_0, POS_QUERY, POS_QUERY_LEN);
    while ( uart_read_bytes(UART_NUM_0, &buffer[i], 1, portMAX_DELAY) > 0 )
    {
        if (buffer[i] == 'R' || ++i > 31)
			break;
    }

    char* begin = strchr((char*)buffer, ';');
    char* end = strchr((char*)buffer, 'R');
    
    if (!begin || !end)
        return 0;

    for (uint8_t factor = 1; factor <= 100; factor *= 10)
    {
        char chr = *(--end);
        if (chr == ';')
            break;
        if (chr < '0' || chr > '9')
            return 0;

        switch (chr)
        {
            case '0':
                break;
            case '1':
                current_column += 1*factor; break;
            case '2':
                current_column += 2*factor; break;
            case '3':
                current_column += 3*factor; break;
            case '4':
                current_column += 4*factor; break;
            case '5':
                current_column += 5*factor; break;
            case '6':
                current_column += 6*factor; break;
            case '7':
                current_column += 7*factor; break;
            case '8':
                current_column += 8*factor; break;
            case '9':
                current_column += 9*factor; break;
        }
    }

    return current_column;
}

static uint16_t get_terminal_width(uint8_t initial_column)
{
	uint16_t width;
	
	uart_write_bytes(UART_NUM_0, R_EDGE, R_EDGE_LEN);
	width = get_current_column();
	
	uart_write_bytes(UART_NUM_0, L_EDGE, L_EDGE_LEN);
	while (--initial_column)
		uart_write_bytes(UART_NUM_0, R_ARROW, R_ARROW_LEN);
	
	return width;
}

static uint8_t shift_viewport_left(input_line* inpl, uint16_t terminal_width, uint8_t pos)
{
	cont* buffer = &inpl->buffer;
	uint8_t viewport_width = terminal_width - inpl->first_column + 1;
	uint8_t cols = 0;
	uint8_t byte_shift = 0;
	uint8_t char_len;
	
	while (viewport_width-- && pos)
	{
		char_len = get_char_len_backwards(cont_get(buffer, pos - 1));
		byte_shift += char_len;
		pos -= char_len;
		cols++;
	}
	
	inpl->viewport_begin -= byte_shift;
	
	return cols;
}

static redraw_status redraw(input_line* inpl, uint16_t terminal_width, uint8_t col_position)
{
	if (inpl->echo_mode == SUPPRESS)
		return (redraw_status){0,0};
	
	uint8_t first_column = inpl->first_column;
	uint8_t viewport_begin = inpl->viewport_begin;
	uint8_t viewport_width = terminal_width - first_column + 1;
	cont* buffer = &inpl->buffer;
	
	uint8_t pos = viewport_begin;
	uint8_t end_of_buffer = buffer->count - 1;
	uint8_t column_count = 0;
	uint8_t byte_count = 0;
	uint8_t next_char_len;
	
	while ( column_count < viewport_width && pos <= end_of_buffer )
	{
		next_char_len = get_char_len_forwards(cont_get(buffer, pos));
		byte_count += next_char_len;
		pos += next_char_len;
		column_count++;
	}
	
	uart_write_bytes(UART_NUM_0, L_EDGE, L_EDGE_LEN);
	while (--first_column)
		uart_write_bytes(UART_NUM_0, R_ARROW, R_ARROW_LEN);
	
	uart_write_bytes(UART_NUM_0, CLEAR_TO_EOL, CLEAR_TO_EOL_LEN);
	
	if (inpl->echo_mode == ECHO)
	{
		uint8_t* ptr = cont_get(buffer, viewport_begin);
		if (ptr)
			uart_write_bytes(UART_NUM_0, ptr, byte_count);
	}
	else // echo_mode == MASK
	{
		uint8_t cols = column_count;
		while (cols--)
			uart_write_bytes(UART_NUM_0, "*", 1);
	}
		
	if (col_position)
	{
		uart_write_bytes(UART_NUM_0, L_EDGE, L_EDGE_LEN);
		while (--col_position)
			uart_write_bytes(UART_NUM_0, R_ARROW, R_ARROW_LEN);
	}
	
	return (redraw_status){byte_count, column_count};
}

static void on_backspace_pressed(input_line* inpl)
{
	uint8_t cursor = inpl->cursor;
	if (!cursor)
		return;
	
	cont* buffer = &inpl->buffer;
	uint8_t bytes_to_be_deleted = get_char_len_backwards(cont_get(buffer, cursor - 1));
	uint8_t initial_position = cursor;
	cursor -= bytes_to_be_deleted;
	
	cont_cut(buffer, cursor, bytes_to_be_deleted);
	inpl->cursor = cursor;
	
	if (inpl->echo_mode == SUPPRESS)
		return;
	
	uint8_t current_column = get_current_column();
	uint16_t terminal_width = get_terminal_width(current_column);
	uint8_t first_column = inpl->first_column;
	if (current_column == first_column)
	{	
		uint8_t cols_behind = shift_viewport_left(inpl, terminal_width, initial_position);
		uint8_t col_pos = first_column - 1 + cols_behind;
		
		redraw(inpl, terminal_width, col_pos);
	}
	else
	{	
		if (current_column == first_column + 1 && cursor == buffer->count)
		{
			if (inpl->viewport_begin == 0) // guard against fault on cursor - 1 < 0
			{							   // in shift_viewport_left
				uart_write_bytes(UART_NUM_0, L_EDGE, L_EDGE_LEN);
				while (--first_column)
					uart_write_bytes(UART_NUM_0, R_ARROW, R_ARROW_LEN);
				
				uart_write_bytes(UART_NUM_0, CLEAR_TO_EOL, CLEAR_TO_EOL_LEN);
				return;
			}
				
			uint8_t cols_behind = shift_viewport_left(inpl, terminal_width - 1, cursor);
			uint8_t col_pos = first_column + cols_behind;
			
			redraw(inpl, terminal_width, col_pos);
		}
		else
			redraw(inpl, terminal_width, current_column - 1);
	}
	
}

static void on_up_arrow_pressed(input_line* inpl)
{
	char** history = inpl->history;
	if (!history)
		return;
	if (inpl->echo_mode != ECHO)
		return;
	
	uint8_t history_index = inpl->history_index;
	
	if (++history_index == HISTORY_CAPACITY)
		return;
	
	char* hist_entry_ptr = history[history_index];
	
	if ( !hist_entry_ptr[0] )
		return;
	
	cont* buffer = &inpl->buffer;
	uint8_t hist_entry_len = strlen(hist_entry_ptr);
	
	if (buffer->count)
		cont_clear(buffer);
 	else if (!inpl->first_column)
		inpl->first_column = get_current_column();
	
	cont_append(buffer, hist_entry_ptr, hist_entry_len);
	uint16_t terminal_width = get_terminal_width(1);
	
	inpl->viewport_begin = 0;
	redraw_status status = redraw(inpl, terminal_width, DONT_MOVE);
	uint8_t byte_count = status.byte_count;
	uint8_t buffer_count = buffer->count;
	
	if ( buffer_count > byte_count )
	{	
		uint8_t first_column = inpl->first_column;
		uart_write_bytes(UART_NUM_0, L_EDGE, L_EDGE_LEN);
		while (--first_column)
			uart_write_bytes(UART_NUM_0, R_ARROW, R_ARROW_LEN);
		inpl->cursor = 0;
	}
	else
	{	
		if ( status.column_count == terminal_width - inpl->first_column + 1 )
			byte_count--;
		inpl->cursor = byte_count;
	}
		
	inpl->history_index = history_index;
}

static void on_down_arrow_pressed(input_line* inpl)
{
	char** history = inpl->history;
	if (!history)
		return;
	if (inpl->echo_mode != ECHO)
		return;
		
	uint8_t history_index = inpl->history_index;
	if (history_index == UINT8_MAX)
		return;
	if (history_index == 0)
		return;
	
	history_index--;
	cont* buffer = &inpl->buffer;
	char* hist_entry_ptr = history[history_index];
	uint8_t hist_entry_len = strlen(hist_entry_ptr);
	
	cont_clear(buffer);
	cont_append(buffer, hist_entry_ptr, hist_entry_len);
	
	uint16_t terminal_width = get_terminal_width(1);

	inpl->viewport_begin = 0;
	redraw_status status = redraw(inpl, terminal_width, DONT_MOVE);
	uint8_t byte_count = status.byte_count;
	uint8_t buffer_count = buffer->count;
	
	if ( buffer_count > byte_count )
	{	
		uint8_t first_column = inpl->first_column;
		uart_write_bytes(UART_NUM_0, L_EDGE, L_EDGE_LEN);
		while (--first_column)
			uart_write_bytes(UART_NUM_0, R_ARROW, R_ARROW_LEN);
		inpl->cursor = 0;
	}
	else
	{	
		if ( status.column_count == terminal_width - inpl->first_column + 1 )
			byte_count--;
		inpl->cursor = byte_count;
	}
	
	inpl->history_index = history_index;
}

static void on_right_arrow_pressed(input_line* inpl)
{
	uint8_t cursor = inpl->cursor;
	cont* buffer = &inpl->buffer;
	if (cursor == buffer->count)
		return;
	
	uint8_t initial_pos = cursor;
		
	uint8_t char_len = get_char_len_forwards(cont_get(buffer, cursor));
	cursor += char_len;
	
	if (inpl->echo_mode != SUPPRESS)
	{
		uint8_t current_column = get_current_column();
		uint16_t terminal_width = get_terminal_width(current_column);
		uint8_t count = buffer->count;
		
		if ( initial_pos <= count - 1 && current_column == terminal_width )
		{
			inpl->viewport_begin += char_len;
			redraw(inpl, terminal_width, DONT_MOVE);
		}
		else
			uart_write_bytes(UART_NUM_0, R_ARROW, R_ARROW_LEN);
	}
	
	inpl->cursor = cursor;
}

static void on_left_arrow_pressed(input_line* inpl)
{
	uint8_t cursor = inpl->cursor;
	if (!cursor)
		return;
	
	uint8_t initial_pos = cursor;
	cont* buffer = &inpl->buffer;
	
	uint8_t prev_char_len = get_char_len_backwards(cont_get(buffer, cursor-1));
	cursor -= prev_char_len;
	
	if (inpl->echo_mode != SUPPRESS)
	{
		uint8_t first_column = inpl->first_column;
		if ( initial_pos && get_current_column() == first_column )
		{	
			inpl->viewport_begin = cursor;
			redraw(inpl, get_terminal_width(first_column), first_column);
		}
		else
			uart_write_bytes(UART_NUM_0, L_ARROW, L_ARROW_LEN);
	}
	
	inpl->cursor = cursor;
}

static void move_cursor_one_word_backward(input_line* inpl)
{
	uint8_t* prev;
	uint8_t* preprev;
	uint8_t cursor = inpl->cursor;
	cont* buffer = &inpl->buffer;
	uint8_t shifts = 0;
	
	prev = cont_get(buffer, cursor - 1);
	if (!prev)
		return;
	
	while (1)
	{
		shifts++;
		cursor -= get_char_len_backwards(prev);
		
		if ( !(preprev = cont_get(buffer, cursor - 1)) )
			break;
		if (*preprev == ' ' && *prev != ' ')
			break;
		
		prev = preprev;
	}
	
	if (cursor < inpl->viewport_begin)
	{
		inpl->viewport_begin = cursor;
		redraw(inpl, get_terminal_width(get_current_column()), inpl->first_column);
	}
	else
	{
		while (shifts--)
			uart_write_bytes(UART_NUM_0, L_ARROW, L_ARROW_LEN);
	}
	
	inpl->cursor = cursor;
	return;
}

static void move_cursor_one_word_forward(input_line* inpl)
{
	uint8_t* first;
	uint8_t* second;
	cont* buffer = &inpl->buffer;
	uint8_t* addr = (uint8_t*)buffer->addr;
	uint8_t cursor = inpl->cursor;
	uint8_t shifts = 0;
	
	first = cont_get(buffer, cursor);
	if (!first)
		return;
	
	uint8_t viewport_begin = inpl->viewport_begin;
	uint8_t current_column = get_current_column();
	uint16_t terminal_width = get_terminal_width(current_column); 
	uint8_t cols_to_the_edge = terminal_width - current_column;
	
	while (1)
	{	
		shifts++;
		cursor += get_char_len_forwards(first);
		
		if (shifts > cols_to_the_edge)
			viewport_begin += get_char_len_forwards(addr + viewport_begin);
	
		if ( !(second = cont_get(buffer, cursor)) )
			break;
		if (*first != ' ' && *second == ' ')
			break;
		
		first = second;
	}
	
	if ( current_column + shifts > terminal_width )
	{
		inpl->viewport_begin = viewport_begin;
		redraw(inpl, terminal_width, DONT_MOVE);
	}
	else
	{
		while (shifts--)
			uart_write_bytes(UART_NUM_0, R_ARROW, R_ARROW_LEN);
	}
	
	inpl->cursor = cursor;
}

static inline void print_stars(uint8_t count)
{
	while (count--)
		uart_write_bytes(UART_NUM_0, "*", 1);
}

static inline uint8_t get_vp_byte_count(uint8_t col_span, uint8_t* ptr)
{
	uint8_t byte_count = 0;	
	uint8_t prev_char_len;
	
	while (col_span--)
	{
		prev_char_len = get_char_len_backwards(ptr);
		byte_count += prev_char_len;
		ptr -= prev_char_len;
	}
	
	return byte_count;
}

static inline uint8_t count_columns(uint8_t* buffer, uint8_t buffer_size)
{
	uint8_t count = 0;
	uint8_t i = 0;
	
	while (i < buffer_size)
	{
		i += get_char_len_forwards(&buffer[i]);
		count++;
	}
	
	return count;
}

static uint8_t input(input_line* inpl, uint8_t* input_buffer, uint8_t input_buffer_size)
{
	uint8_t cursor = inpl->cursor;
	char echo_mode = inpl->echo_mode;
	cont* buffer = &inpl->buffer;
	
	uint8_t current_column = get_current_column();
	uint16_t terminal_width = get_terminal_width(current_column);
	uint8_t columns_to_be_added = count_columns(input_buffer, input_buffer_size);
	
	if (!inpl->first_column && echo_mode != SUPPRESS)
		inpl->first_column = current_column;
	
	if (cursor == buffer->count)
	{
		cont_append(buffer, input_buffer, input_buffer_size);
		cursor += input_buffer_size;
		inpl->cursor = cursor;
		
		if (echo_mode == SUPPRESS)
			return cursor;
		
		if (current_column == terminal_width || current_column + columns_to_be_added >= terminal_width)
		{
			uint8_t column_span = terminal_width - inpl->first_column; /////////!!!!
			uint8_t viewport_len = get_vp_byte_count(column_span, cont_get(buffer, cursor-1));

			inpl->viewport_begin = cursor - viewport_len;
			redraw(inpl, terminal_width, DONT_MOVE);
		}
		else if (echo_mode == ECHO)
			uart_write_bytes(UART_NUM_0, input_buffer, input_buffer_size);
		else if (echo_mode == MASK)
			print_stars(columns_to_be_added);
		
		return cursor;
	}
	else // inserting text inbetween
	{
		uint8_t init_curs_pos = inpl->cursor;
		
		cont_insert_range(buffer, cursor, input_buffer, input_buffer_size);
		cursor += input_buffer_size;
		inpl->cursor = cursor;
		
		if (echo_mode == SUPPRESS)
			return cursor;
		
		if (current_column + columns_to_be_added > terminal_width)
		{
			inpl->viewport_begin += current_column + columns_to_be_added - terminal_width;
			redraw(inpl, terminal_width, DONT_MOVE);
		}
		else // typing cursor doesnt go beyond the viewport
		{
			uint8_t column_span = count_columns(buffer->addr, buffer->count);
			uint8_t typing_cursor_pos = current_column + columns_to_be_added;
			
			// if buffer len exceeds viewport
			if (column_span > terminal_width - inpl->first_column + 1)
				redraw(inpl, terminal_width, typing_cursor_pos);
			else if (echo_mode == ECHO)
			{
				uart_write_bytes(UART_NUM_0, cont_get(buffer, init_curs_pos), buffer->count - init_curs_pos);
				uart_write_bytes(UART_NUM_0, L_EDGE, L_EDGE_LEN);
				
				while (--typing_cursor_pos)
					uart_write_bytes(UART_NUM_0, R_ARROW, R_ARROW_LEN);
			}
			else if (echo_mode == MASK)
			{
				uart_write_bytes(UART_NUM_0, L_EDGE, L_EDGE_LEN);
				uint16_t first_column = inpl->first_column;
				while (--first_column)
					uart_write_bytes(UART_NUM_0, R_ARROW, R_ARROW_LEN);
					
				print_stars(column_span);
				uart_write_bytes(UART_NUM_0, L_EDGE, L_EDGE_LEN);
				
				while (--typing_cursor_pos)
					uart_write_bytes(UART_NUM_0, R_ARROW, R_ARROW_LEN);
			}
			
		}
		
		return cursor;
	}
}

int read_input(input_line* inpl)
{
	char echo_mode = inpl->echo_mode;
	cont* buffer = &inpl->buffer;
	
	uint8_t input_buffer[CMD_LINE_SIZE];
	int input_buffer_size = 0;
	uint8_t byte;
	
	while (1)
	{
		input_buffer_size = read_byte_sequence(input_buffer); 
		if (input_buffer_size <= 0)
			return input_buffer_size;
			
		if (input_buffer_size == 1)
		{
			byte = input_buffer[0];
			if (byte == CTRL_C) // CTRL+C
			{ 
				uart_write_bytes(UART_NUM_0, NEW_LINE, NEW_LINE_LEN);
				return CTRL_C_PRESSED;
			}
			if (byte == LF || byte == CR) // /n || /r
				break;
			if (byte == ESC) // just ESC pressed
				continue;
			if (byte == BACKSPACE || byte == DEL) // backspace
			{
				on_backspace_pressed(inpl);
				continue;
			} 
			
		}
		else if (input_buffer_size == 2)
		{
			byte = input_buffer[0];
			if (byte == LF || byte == CR)
				break;
		}
		else if (input_buffer_size == 3)
		{
			byte = input_buffer[0];
			if (byte == ESC) // it's an escape sequence
			{
				if ( (byte = input_buffer[1]) == '[' || byte == 'O') // arrow sequence
				{
					if ( (byte = input_buffer[2]) == 'A') // arrow up
					{	
						if (echo_mode != ECHO)
							continue;
						
						on_up_arrow_pressed(inpl);
						continue;
					}
					if ( byte == 'B') // arrow down
					{
						if (echo_mode != ECHO)
							continue;
							
						on_down_arrow_pressed(inpl);
						continue;	
					}
					if ( byte == 'C') // arrow right
					{
						on_right_arrow_pressed(inpl);
						continue;
					}
					if ( byte == 'D') // arrow left
				    {
						on_left_arrow_pressed(inpl);
						continue;
					}
				}
			}
		}
		else // input_buffer_size >= 4 bytes
		{
			if (input_buffer[0] == ESC) // escape sequence
			{
				if (echo_mode == SUPPRESS)
					continue;
				
				byte = input_buffer[input_buffer_size-1];
				
				if (byte == 'C') // right arrow final byte
					move_cursor_one_word_forward(inpl);
				else if (byte == 'D') // left arrow final byte
					move_cursor_one_word_backward(inpl);
				
				continue;
			}
		}
		
		if (buffer->count + input_buffer_size < buffer->max_capacity)
		{
			if (inpl->is_restricted)
				sanitize_utf8(input_buffer, input_buffer_size, '?');
			
			input(inpl, input_buffer, input_buffer_size);
		}
	}
	
	uart_write_bytes(UART_NUM_0, NEW_LINE, NEW_LINE_LEN);
	buffer->addr[buffer->count] = '\0';
	return (int)buffer->count;
}

uint8_t update_history(input_line* inpl)
{
	char** history = inpl->history;
	cont* buffer = &inpl->buffer;

	if (!buffer->addr)
		return INPUT_LINE_ALREADY_FREED;
	if (!history)
		return NO_HISTORY;
	if (!buffer->count)
		return EMPTY_CMD_LINE;
	
	memmove(history[1], history[0], ( (HISTORY_CAPACITY-1)*CMD_LINE_SIZE ));
	memcpy(history[0], buffer->addr, buffer->count);
	history[0][buffer->count] = '\0';
	
	return 0;
}
