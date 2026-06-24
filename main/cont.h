#ifndef CONTLIB_CONT_H
#define CONTLIB_CONT_H

#include <stddef.h>

#define CONTLIB_UNIT_BUFF_SIZE 256

enum error_codes
{
	INVALID_INDEX = 1,
	SIZE_OVERFLOW,
	NULL_ARRAY_POINTER,
	REALLOC_FAILURE,
	MALLOC_FAILURE,
	NULL_ITEM_POINTER,
	NULL_ELEMENT_COUNT,
	NULL_CAPACITY,
	MAX_CAPACITY_EXCEEDED,
	COUNT_LARGER_THAN_CURRENT_COUNT,
	INVALID_GROWTH_FACTOR_VALUE,
	CONT_ALREADY_FREED,
	EMPTY_CONT,
	UNIT_MISMATCH,
	SAME_CONT,
	INVALID_RANGE,
	CONT_IS_INVALID
};

typedef struct vtable cont_methods;

typedef struct
{
    size_t unit;
    size_t count;
    size_t capacity;
    size_t max_capacity;
    double growth_factor;
    unsigned char* addr;
    cont_methods* m;
} cont;

struct vtable
{
	int (*is_valid)(cont*);
	int (*set_count)(cont*, size_t);
	int (*set_capacity)(cont*, size_t);
	int (*set_max_capacity)(cont*, size_t);
	int (*set_growth_factor)(cont*, double);
	void* (*get)(cont*, size_t);
	int (*set)(cont*, size_t, void*);
	int (*cv)(cont*, size_t, void*, size_t);
	void* (*pop)(cont*);
	int (*push)(cont*, void*);
	int (*push_front)(cont*, void*);
	int (*write)(cont*, size_t, void*, size_t);
	int (*insert)(cont*, size_t, void*);
	int (*insert_range)(cont*, size_t, void*, size_t);
	int (*append)(cont*, void*, size_t);
	int (*prepend)(cont*, void*, size_t);
	int (*set_space)(cont*, size_t);
	int (*shrink)(cont*);
	int (*remove)(cont*, size_t);
	int (*cut)(cont*, size_t, size_t);
	int (*reverse)(cont*);
	int (*clear)(cont*);
	int (*set_blank)(cont*, size_t, size_t);
	int (*collapse)(cont*);
	int (*free)(cont*);
	cont (*clone)(cont*);
	int (*extend)(cont*, cont*);
	cont (*split)(cont*, size_t);
	cont (*sub)(cont*, size_t, size_t);
	int (*grow)(cont*, size_t);
};

extern const cont INVALID_CONT;

cont cont_new(size_t capacity, size_t unit);

int cont_is_valid(cont* cont_);

int cont_set_count(cont* cont_, size_t count);

int cont_set_capacity(cont* cont_, size_t capacity);

int cont_set_max_capacity(cont* cont_, size_t max_size);

int cont_set_growth_factor(cont* cont_, double growth_factor);

void* cont_get(cont* cont_, size_t index);

int cont_set(cont* cont_, size_t index, void* item);

int cont_cv(cont* cont_, size_t index, void* buffer, size_t n);

void* cont_pop(cont* cont_);

int cont_push(cont* cont_, void* item);

int cont_push_front(cont* cont_, void* item);

int cont_write(cont* cont_, size_t index, void* arr, size_t num);

int cont_insert(cont* cont_, size_t index, void* item);

int cont_insert_range(cont* cont_, size_t index, void* arr, size_t num);

int cont_append(cont* cont_, void* arr, size_t n);

int cont_prepend(cont* cont_, void* arr, size_t n);

int cont_set_space(cont* cont_, size_t n);

int cont_shrink(cont* cont_);

int cont_remove(cont* cont_, size_t index);

int cont_cut(cont* cont_, size_t index, size_t n);

int cont_reverse(cont* cont_);

int cont_clear(cont* cont_);

int cont_set_blank(cont* cont_, size_t position, size_t n);

int cont_collapse(cont* cont_);

int cont_free(cont* cont_);

cont cont_clone(cont* cont_);

int cont_extend(cont* cont_, cont* cont_2);

cont cont_split(cont* cont_, size_t index);

cont cont_sub(cont* cont_, size_t index, size_t n);

int cont_grow(cont* cont_, size_t required_capacity);

#endif
