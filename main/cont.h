#ifndef CONTLIB_CONT_H
#define CONTLIB_CONT_H

#include <stddef.h>
#include <stdbool.h>

#define cont_UNIT_BUFF_SIZE 256

#define cont_NO_LIMIT 0
#define cont_ALL 0

#define cont_GF 2.0

#define cont_GF_LBOUND 1.0
#define cont_GF_UBOUND 10.0

#define cont_NEW(type, capacity) 						  		\
		cont_new(capacity, sizeof(type), _Alignof(type))

#define cont_INDEX(cnt, index) 									\
				  ( (index) >= (cnt)->count ) ? false : true;

#define cont_ITEM(cnt, index, type) 					  		\
				 ( (type*)((cnt)->addr) )[index]

#define cont_PUSH(cnt, type) 							  		\
				 ( (type*)((cnt)->addr) )[(cnt)->count++]


enum error_codes
{
	INVALID_INDEX = 1,
	SIZE_OVERFLOW,
	NULL_ARRAY_POINTER,
	REALLOC_FAILURE,
	MALLOC_FAILURE,
	ALIGNED_ALLOC_FAILURE,
	NULL_ITEM_POINTER,
	NULL_ELEMENT_COUNT,
	NULL_CAPACITY,
	MAX_CAPACITY_EXCEEDED,
	INVALID_GROWTH_FACTOR_VALUE,
	CONT_ALREADY_FREED,
	EMPTY_CONT,
	UNIT_MISMATCH,
	SAME_CONT,
	INVALID_RANGE,
	CONT_IS_INVALID,
	CONT_IS_NULL,
	BUFFER_OVERLAP,
	INVALID_REQUIRED_CAPACITY
};

typedef struct
{
    size_t unit;
	size_t alignment;
    size_t count;
    size_t capacity;
    size_t max_capacity;
    double growth_factor;
    unsigned char* addr;

} cont;

extern const cont INVALID_CONT;

cont cont_new(size_t capacity, size_t unit, size_t alignment);
int cont_is_valid(cont* cnt);
int cont_set_count(cont* cnt, size_t count);
int cont_set_capacity(cont* cnt, size_t capacity);
int cont_set_max_capacity(cont* cnt, size_t max_size);
int cont_set_growth_factor(cont* cnt, double growth_factor);
int cont_lock(cont* cnt);
void* cont_get(cont* cnt, size_t index);
int cont_set(cont* cnt, size_t index, void* item);
int cont_cv(cont* cnt, size_t index, void* buffer, size_t n);
void* cont_pop(cont* cnt);
int cont_mkroom(cont* cnt, size_t index);
int cont_push(cont* cnt, void* item);
int cont_push_front(cont* cnt, void* item);
int cont_write(cont* cnt, size_t index, void* arr, size_t num_of_items);
int cont_insert(cont* cnt, size_t index, void* item);
int cont_insert_range(cont* cnt, size_t index, void* arr, size_t num_of_items);
int cont_append(cont* cnt, void* arr, size_t n);
int cont_prepend(cont* cnt, void* arr, size_t n);
int cont_set_space(cont* cnt, size_t n);
int cont_shrink(cont* cnt);
int cont_remove(cont* cnt, size_t index);
int cont_remove_range(cont* cnt, size_t index, size_t n);
int cont_reverse(cont* cnt);
int cont_clear(cont* cnt);
int cont_set_blank(cont* cnt, size_t position, size_t n);
int cont_collapse(cont* cnt);
int cont_free(cont* cnt);
cont cont_clone(cont* cnt);
int cont_extend(cont* cnt, cont* cnt2);
cont cont_split(cont* cnt, size_t index);
cont cont_sub(cont* cnt, size_t index, size_t n_elements);
int cont_grow(cont* cnt, size_t required_capacity);
int cont_ensure(cont* cnt, size_t space);

#endif
