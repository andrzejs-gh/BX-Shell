#ifndef FUNCTION_TABLE_LIB_H
#define FUNCTION_TABLE_LIB_H

#include <stddef.h>

#define P 31
#define M 1000000007
#define LOAD_FACTOR 0.7

enum
{
	TABLE_ALREADY_FREED = 1,
	ALLOCATION_FAILURE
};

typedef int (*f_ptr)(int, char**);

typedef struct
{
	char* key;
	f_ptr function;
} ftable_entry;

typedef struct
{
	ftable_entry* table;
	size_t table_size;
	size_t table_count;
} ftable;

extern const ftable INVALID_FTABLE;

ftable ftable_new(size_t initial_capacity);
int ftable_free(ftable* ft);
ftable_entry* ftable_get(ftable* ft, char* key);
int ftable_set(ftable* ft, ftable_entry entry);

#endif
