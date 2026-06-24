#ifndef BXSh_CONSTANTS_H
#define BXSh_CONSTANTS_H

#define CMD_LINE_SIZE 255
#define HISTORY_CAPACITY 5

#define CMD_LINE_DEFAULT_TITLE "BXSh"

#define ECHO 'e'
#define MASK 'm'
#define SUPPRESS 's'

#define DONT_MOVE 0

#define INVALID_INPUT_LINE (input_line){INVALID_CONT,0,0,0,0,0,0,0}

// control keys

#define CTRL_C 0x03
#define ESC 0x1B
#define BACKSPACE '\b'
#define DEL 0x7F
#define LF '\n'
#define CR '\r'

// control sequences

#define NEW_LINE "\r\n"
#define NEW_LINE_LEN (sizeof(NEW_LINE) - 1)

#define POS_QUERY "\x1b[6n"
#define POS_QUERY_LEN (sizeof(POS_QUERY) - 1)

#define R_ARROW "\x1b[C"
#define R_ARROW_LEN (sizeof(R_ARROW) - 1)

#define L_ARROW "\x1b[D"
#define L_ARROW_LEN (sizeof(L_ARROW) - 1)

#define DOWN_ARROW "\x1b[B"
#define DOWN_AROW_LEN (sizeof(DOWN_ARROW) - 1)

#define UP_ARROW "\x1b[A"
#define UP_ARROW_LEN (sizeof(UP_ARROW) - 1)

#define CTRL_R_ARROW "\x1b[1;5C"
#define CTRL_R_ARROW_LEN (sizeof(CTRL_R_ARROW) - 1)

#define CTRL_L_ARROW "\x1b[1;5D"
#define CTRL_L_ARROW_LEN (sizeof(CTRL_L_ARROW) - 1)

#define CTRL_DOWN_ARROW "\x1b[1;5B"
#define CTRL_DOWN_ARROW_LEN (sizeof(CTRL_DOWN_ARROW) - 1)

#define CTRL_UP_ARROW "\x1b[1;5A" 
#define CTRL_UP_ARROW_LEN (sizeof(CTRL_UP_ARROW) - 1)

#define R_EDGE "\x1b[9999C"
#define R_EDGE_LEN (sizeof(R_EDGE) - 1)

#define L_EDGE "\x1b[1G"
#define L_EDGE_LEN (sizeof(L_EDGE) - 1)

#define DEL_ROW "\x1b[M"
#define DEL_ROW_LEN (sizeof(DEL_ROW) - 1)

#define CLEAR_TO_EOL "\x1b[K"
#define CLEAR_TO_EOL_LEN (sizeof(CLEAR_TO_EOL) - 1)

// error codes

enum
{
	CTRL_C_PRESSED = -2,
	INPUT_LINE_ALREADY_FREED = 1,
	NO_HISTORY,
	INCORECT_ECHO_MODE,
	HISTORY_LAST_ENTRY,
	HISTORY_ENTRY_EMPTY,
	NOT_IN_HISTORY,
	HISTORY_FIRST_ENTRY,
	EMPTY_CMD_LINE
};

#endif
