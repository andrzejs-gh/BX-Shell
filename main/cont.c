#include "cont.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

int inv_set_count(cont* c, size_t s){return CONT_IS_INVALID;}
int inv_set_capacity(cont* c, size_t s){return CONT_IS_INVALID;}
int inv_set_max_capacity(cont* c, size_t s){return CONT_IS_INVALID;}
int inv_set_growth_factor(cont* c, double d){return CONT_IS_INVALID;}
int inv_set(cont* c, size_t s, void* v){return CONT_IS_INVALID;}
int inv_cv(cont* c, size_t s, void* v, size_t s_){return CONT_IS_INVALID;}
int inv_push(cont* c, void* v){return CONT_IS_INVALID;}
int inv_push_front(cont* c, void* v){return CONT_IS_INVALID;}
int inv_write(cont* c, size_t s, void* v, size_t s_){return CONT_IS_INVALID;}
int inv_insert(cont* c, size_t s, void* v){return CONT_IS_INVALID;}
int inv_insert_range(cont* c, size_t s, void* v, size_t s_){return CONT_IS_INVALID;}
int inv_append(cont* c, void* v, size_t s){return CONT_IS_INVALID;}
int inv_prepend(cont* c, void* v, size_t s){return CONT_IS_INVALID;}
int inv_set_space(cont* c, size_t s){return CONT_IS_INVALID;}
int inv_shrink(cont* c){return CONT_IS_INVALID;}
int inv_remove(cont* c, size_t s){return CONT_IS_INVALID;}
int inv_cut(cont* c, size_t s, size_t s_){return CONT_IS_INVALID;}
int inv_reverse(cont* c){return CONT_IS_INVALID;}
int inv_clear(cont* c){return CONT_IS_INVALID;}
int inv_set_blank(cont* c, size_t s, size_t s_){return CONT_IS_INVALID;}
int inv_collapse(cont* c){return CONT_IS_INVALID;}
int inv_extend(cont* c, cont* c_){return CONT_IS_INVALID;}
int inv_grow(cont* c, size_t s){return CONT_IS_INVALID;}

void* inv_get(cont* c, size_t s){return NULL;}
void* inv_pop(cont* c){return NULL;}

cont inv_clone(cont* c){return INVALID_CONT;}
cont inv_split(cont* c, size_t s){return INVALID_CONT;}
cont inv_sub(cont* c, size_t s, size_t s_){return INVALID_CONT;}

cont_methods std_cont_methods = {
	.is_valid = cont_is_valid,
	.set_count = cont_set_count,
	.set_capacity = cont_set_capacity,
	.set_max_capacity = cont_set_max_capacity,
	.set_growth_factor = cont_set_growth_factor,
	.get = cont_get,
	.set = cont_set,
	.cv = cont_cv,
	.pop = cont_pop,
	.push = cont_push,
	.push_front = cont_push_front,
	.write = cont_write,
	.insert = cont_insert,
	.insert_range = cont_insert_range,
	.append = cont_append,
	.prepend = cont_prepend,
	.set_space = cont_set_space,
	.shrink = cont_shrink,
	.remove = cont_remove,
	.cut = cont_cut,
	.reverse = cont_reverse,
	.clear = cont_clear,
	.set_blank = cont_set_blank,
	.collapse = cont_collapse,
	.free = cont_free,
	.clone = cont_clone,
	.extend = cont_extend,
	.split = cont_split,
	.sub = cont_sub,
	.grow = cont_grow
};

cont_methods invalid_cont_methods = {
	.is_valid = cont_is_valid,
	.set_count = inv_set_count,
	.set_capacity = inv_set_capacity,
	.set_max_capacity = inv_set_max_capacity,
	.set_growth_factor = inv_set_growth_factor,
	.get = inv_get,
	.set = inv_set,
	.cv = inv_cv,
	.pop = inv_pop,
	.push = inv_push,
	.push_front = inv_push_front,
	.write = inv_write,
	.insert = inv_insert,
	.insert_range = inv_insert_range,
	.append = inv_append,
	.prepend = inv_prepend,
	.set_space = inv_set_space,
	.shrink = inv_shrink,
	.remove = inv_remove,
	.cut = inv_cut,
	.reverse = inv_reverse,
	.clear = inv_clear,
	.set_blank = inv_set_blank,
	.collapse = inv_collapse,
	.free = cont_free,
	.clone = inv_clone,
	.extend = inv_extend,
	.split = inv_split,
	.sub = inv_sub,
	.grow = inv_grow
};

const cont INVALID_CONT = {
    .unit = 0,
    .count = 0,
	.capacity = 0,
	.max_capacity = 0,
	.growth_factor = 0.0,
	.addr = NULL,
	.m = &invalid_cont_methods
};

cont cont_new(size_t capacity, size_t unit)
{
    if (!capacity || !unit || capacity > SIZE_MAX / unit)
		return INVALID_CONT;
	
    size_t size = capacity * unit;
    void* addr = malloc(size);
    if (!addr)
        return INVALID_CONT;
        
    cont cont_ = {
        .unit = unit,
        .count = 0,
        .capacity = capacity,
        .max_capacity = 0,
        .growth_factor = 2.0,
        .addr = addr,
        .m = &std_cont_methods
    };
    
    return cont_;
}

int cont_is_valid(cont* cont_)
{
	if (!cont_ || cont_ == &INVALID_CONT)
		return 0;
	
	size_t capacity = cont_->capacity;
	double growth_factor = cont_->growth_factor;
	
	if (!cont_->unit)
		return 0;
	if (!capacity)
		return 0;
	if (cont_->count > capacity)
		return 0;
	if (cont_->max_capacity && capacity > cont_->max_capacity)
		return 0;
	if (!(growth_factor > 1.0) || !(growth_factor <= 10.0) || !isfinite(growth_factor))
		return 0;
	if (!cont_->addr)
		return 0;
		
	return 1;
}

int cont_set_count(cont* cont_, size_t count)
{
	if (count > cont_->count)
		return COUNT_LARGER_THAN_CURRENT_COUNT;
	
	cont_->count = count;
	
	return 0;
}

int cont_set_capacity(cont* cont_, size_t capacity)
{
	if (capacity == cont_->capacity)
		return 0;
	if (!capacity)
		return NULL_CAPACITY;
		
	size_t unit = cont_->unit;
		
	if (capacity > SIZE_MAX / unit)
		return SIZE_OVERFLOW;
		
	size_t max_capacity = cont_->max_capacity;
		
	if (max_capacity && capacity > max_capacity)
		return MAX_CAPACITY_EXCEEDED;
	
	void* ptr = realloc(cont_->addr, capacity*unit);
	if (!ptr)
		return REALLOC_FAILURE;
	
	if (cont_->count > capacity)
		cont_->count = capacity;
	cont_->capacity = capacity;
	cont_->addr = ptr;

	return 0;
}

int cont_set_max_capacity(cont* cont_, size_t max_size)
{
	size_t unit = cont_->unit;
	if (max_size > SIZE_MAX / unit)
		return SIZE_OVERFLOW;
	
	if (!max_size) // 0 = no limit
		cont_->max_capacity = max_size;
	else if (cont_->count > max_size || cont_->capacity > max_size)
	{
		void* ptr = realloc(cont_->addr, max_size*unit);
		if (!ptr)
			return REALLOC_FAILURE;
		
		cont_->addr = ptr;
		cont_->max_capacity = max_size;
		cont_->capacity = max_size;
		if (cont_->count > max_size)
			cont_->count = max_size;
	}
	else
		cont_->max_capacity = max_size;
	
	return 0; 
}

int cont_set_growth_factor(cont* cont_, double growth_factor)
{
	if (!(growth_factor > 1.0) || !(growth_factor <= 10.0) || !isfinite(growth_factor))
		return INVALID_GROWTH_FACTOR_VALUE;
		
	cont_->growth_factor = growth_factor;
	
	return 0;
}

void* cont_get(cont* cont_, size_t index)
{
	if (index >= cont_->count)
		return NULL;
	else
		return cont_->addr+(index*(cont_->unit));
}

int cont_set(cont* cont_, size_t index, void* item)
{
	if (index >= cont_->count)
		return INVALID_INDEX;
	if (!item)
		return NULL_ITEM_POINTER;
	
	size_t unit = cont_->unit;
	
	memcpy(cont_->addr+(index*unit), item, unit);
	
	return 0;
}

int cont_cv(cont* cont_, size_t index, void* buffer, size_t n)
{	
	if (!buffer)
		return NULL_ARRAY_POINTER;
		
	size_t count = cont_->count;
		
	if (index >= count)
		return INVALID_INDEX;
	
	if (!n)
		n = count - index;
	else if (n > count - index)
		return INVALID_RANGE;
	
	size_t unit = cont_->unit;
	
	memcpy(buffer, cont_->addr+(index*unit), n*unit);
	
	return 0;
}

void* cont_pop(cont* cont_)
{
	size_t count = cont_->count;
	if (!count)
		return NULL;
	
	cont_->count--;
	return cont_->addr+(count-1)*cont_->unit;
}

int cont_push(cont* cont_, void* item)
{
	if (!item)
		return NULL_ITEM_POINTER;
	
	size_t count = cont_->count;
	size_t max_capacity = cont_->max_capacity;
	if (max_capacity && count == max_capacity)
		return MAX_CAPACITY_EXCEEDED;
	
	size_t unit = cont_->unit;
	
	if (count == cont_->capacity)
	{		 
		int ret = cont_grow(cont_, count+1);
		if (ret)
			return ret;
	}
	
	memcpy(cont_->addr+count*unit, item, unit);
	cont_->count++;
	
	return 0;	
}

int cont_push_front(cont* cont_, void* item)
{
	if (!item)
		return NULL_ITEM_POINTER;
	
	size_t count = cont_->count;
	size_t max_capacity = cont_->max_capacity;
	
	if (max_capacity && count == max_capacity)
		return MAX_CAPACITY_EXCEEDED;
	
	if (count == cont_->capacity) // if there is no space
	{
		int ret = cont_grow(cont_, count+1);
		if (ret) return ret;
	}

	size_t unit = cont_->unit;
	unsigned char* addr = cont_->addr;
	
	memmove(addr+unit, addr, count*unit);
	memcpy(addr, item, unit);
	
	cont_->count++;
	
	return 0;
}

int cont_write(cont* cont_, size_t index, void* arr, size_t num)
{	
	if (!arr)
        return NULL_ARRAY_POINTER;
    if (!num)
		return NULL_ELEMENT_COUNT;
		
	size_t count = cont_->count;
	
	if (index > count)
		return INVALID_INDEX;
	if (index > SIZE_MAX - num)
		return SIZE_OVERFLOW;
	
	size_t unit = cont_->unit;
	
	if (num > SIZE_MAX / unit)
		return SIZE_OVERFLOW;
	
	size_t size_to_copy = num*unit;
	size_t insertion_range = index + num;
	
    if (insertion_range > cont_->capacity)
	{
		size_t max_capacity = cont_->max_capacity;
		if (max_capacity && insertion_range > max_capacity)
			return MAX_CAPACITY_EXCEEDED;
			
		int ret = cont_grow(cont_, insertion_range);
		if (ret)
			return ret;
			
		cont_->count = insertion_range;
	}
	else if (insertion_range > count)
		cont_->count = insertion_range;
	
	memcpy(cont_->addr+(index*unit), arr, size_to_copy);
    
    return 0;
}

int cont_insert(cont* cont_, size_t index, void* item)
{
	size_t count = cont_->count;
	
	if (!item)
		return NULL_ITEM_POINTER;
	if (index > count)
		return INVALID_INDEX;
		
	size_t max_capacity = cont_->max_capacity;
		
	if (max_capacity && count == max_capacity)
		return MAX_CAPACITY_EXCEEDED;
	
	if (count == cont_->capacity)
	{
		int ret = cont_grow(cont_, count+1);
		if (ret)
			return ret;
	}
	
	size_t unit = cont_->unit;
	unsigned char* position = cont_->addr + index*unit;
	
	memmove(position+unit, position, (count - index)*unit);
	memcpy(position, item, unit);
	cont_->count++;
	
	return 0;
}

int cont_insert_range(cont* cont_, size_t index, void* arr, size_t num)
{
	if (!arr)
        return NULL_ARRAY_POINTER;
    if (!num)
		return NULL_ELEMENT_COUNT;
	
	size_t count = cont_->count;
		
	if (index > count)
		return INVALID_INDEX;
	if (count > SIZE_MAX - num)
		return SIZE_OVERFLOW;
	
	size_t unit = cont_->unit;
	size_t max_capacity = cont_->max_capacity;
	
	if (max_capacity && count + num > max_capacity)
		return MAX_CAPACITY_EXCEEDED;
	if (num > SIZE_MAX / unit)
		return SIZE_OVERFLOW;
	
	size_t new_count = count + num;
		
	if (new_count > cont_->capacity)
	{
		int ret = cont_grow(cont_, new_count);
		if (ret)
			return ret;
	}

	unsigned char* position = cont_->addr + index*unit;
	size_t inserted_size = num*unit;
	
	memmove(position+inserted_size, position, (count - index)*unit);
	memcpy(position, arr, inserted_size);
	cont_->count += num;

	return 0;
}

int cont_append(cont* cont_, void* arr, size_t n)
{
	return cont_write(cont_, cont_->count, arr, n);
}

int cont_prepend(cont* cont_, void* arr, size_t n)
{
	return cont_insert_range(cont_, 0, arr, n);
}

int cont_set_space(cont* cont_, size_t n)
{	
	size_t count = cont_->count;
	if (!count)
		return EMPTY_CONT;
	
	if (count > SIZE_MAX - n)
		return SIZE_OVERFLOW;
		
	size_t new_capacity = count + n;
	size_t max_capacity = cont_->max_capacity;
		
	if (max_capacity && new_capacity > max_capacity)
		return MAX_CAPACITY_EXCEEDED;
		
	size_t unit = cont_->unit;
	
	if (new_capacity > SIZE_MAX / unit)
		return SIZE_OVERFLOW;
	
	void* ptr = realloc(cont_->addr, new_capacity*unit);
	if (!ptr)
		return REALLOC_FAILURE;
	
	cont_->capacity = new_capacity;
	cont_->addr = ptr;
	
	return 0;
}

int cont_shrink(cont* cont_)
{
	return cont_set_space(cont_, 0);
}

int cont_remove(cont* cont_, size_t index)
{
	size_t count = cont_->count;
	if (index >= count)
		return INVALID_INDEX;
	
	size_t unit = cont_->unit;	
	unsigned char* begin = cont_->addr + index*unit;
	
	memmove(begin, begin+unit, (count-1-index)*unit);
	cont_->count--;
		
	return 0;
}

int cont_cut(cont* cont_, size_t index, size_t n)
{
	size_t count = cont_->count;
	
	if (index >= count)
		return INVALID_INDEX;
	
	if (!n)
		n = count - index;
	else if (n > count - index)
		return INVALID_RANGE;
	
	size_t unit = cont_->unit;
	unsigned char* begin = cont_->addr + index*unit;
	unsigned char* end = begin + n*unit;
	
	memmove(begin, end, (count-index-n)*unit);
	cont_->count -= n;

	return 0;
}

int cont_reverse(cont* cont_)
{
	size_t count = cont_->count;
	
	if (!count)
		return EMPTY_CONT;
	if (count == 1)
		return 0;
	
	size_t unit = cont_->unit;
	
	unsigned char* front = cont_->addr;
	unsigned char* back = cont_->addr + (count-1)*unit;
	unsigned char stack_buffer[CONTLIB_UNIT_BUFF_SIZE];
	unsigned char* buffer;
	
	if (unit <= CONTLIB_UNIT_BUFF_SIZE)
		buffer = stack_buffer;
	else
	{
		buffer = malloc(unit);
		if (!buffer)
			return MALLOC_FAILURE;
	}
	
	while (back > front)
	{
		memcpy(buffer, front, unit);
		memcpy(front, back, unit);
		memcpy(back, buffer, unit);
		front += unit;
		back -= unit;
	}
	
	if (buffer != stack_buffer) 
		free(buffer);
	
	return 0;
}

int cont_clear(cont* cont_)
{
	cont_->count = 0;
	return 0;
}

int cont_set_blank(cont* cont_, size_t position, size_t n)
{
	size_t capacity = cont_->capacity;
	
	if (position >= capacity)
		return INVALID_INDEX;
	
	if (!n)
		n = capacity - position;
	else if (n > capacity - position)
		return INVALID_RANGE;
	
	size_t unit = cont_->unit;
	
	memset(cont_->addr + position*unit, 0, n*unit);
	return 0;
}

int cont_collapse(cont* cont_)
{
	int ret = cont_set_capacity(cont_, 1);
	if (ret)
		return ret;
	cont_->count = 0;
	return 0;
}

int cont_free(cont* cont_)
{
	if (!cont_->addr)
		return CONT_ALREADY_FREED;
	
	free(cont_->addr);
	
	*cont_ = INVALID_CONT;
	
	return 0;
}

cont cont_clone(cont* cont_)
{
	size_t unit = cont_->unit;
	size_t mem_size = cont_->capacity*unit;
	
	void* cloned_cont_addr = malloc(mem_size);
	if (!cloned_cont_addr)
		return INVALID_CONT;
	
	size_t size_to_copy = cont_->count*unit;
	memcpy(cloned_cont_addr, cont_->addr, size_to_copy);
	
	return (cont){.unit = unit,
				  .count = cont_->count,
				  .capacity = cont_->capacity,
				  .max_capacity = cont_->max_capacity,
				  .growth_factor = cont_->growth_factor,
				  .addr = cloned_cont_addr};
}

int cont_extend(cont* cont_, cont* cont_2)
{
	if (cont_ == cont_2)
		return SAME_CONT;
	if (cont_->unit != cont_2->unit)
		return UNIT_MISMATCH;
	if (!cont_2->count)
		return 0;
	
	return cont_append(cont_, cont_2->addr, cont_2->count);
}

cont cont_split(cont* cont_, size_t index)
{
	size_t count = cont_->count;
	
	if (!index || index >= count)
		return INVALID_CONT;
	
	size_t unit = cont_->unit;
	
	size_t elements_count = count - index;
	size_t mem_size = elements_count*unit;
	
	void* new_addr = malloc(mem_size);
	if (!new_addr)
		return INVALID_CONT;
	
	memcpy(new_addr, cont_->addr+(index*unit), mem_size);
	if (cont_set_capacity(cont_, index)) //
	{
		free(new_addr);
		return INVALID_CONT;
	}
		
	return (cont){.unit = unit,
				  .count = elements_count,
				  .capacity = elements_count,
				  .max_capacity = cont_->max_capacity,
				  .growth_factor = cont_->growth_factor,
				  .addr = new_addr};
}

cont cont_sub(cont* cont_, size_t index, size_t n)
{
	size_t count = cont_->count;
	
	if (index >= count)
		return INVALID_CONT;

	if (!n)
		n = count - index;
	else if (n > count - index)
		return INVALID_CONT;
	
	size_t unit = cont_->unit;
	size_t size = n*unit;
	
	void* new_addr = malloc(size);
	if (!new_addr)
		return INVALID_CONT;
		
	memcpy(new_addr, cont_->addr+(index*unit), size);
	
	return (cont){.unit = unit,
				  .count = n,
				  .capacity = n,
				  .max_capacity = 0,
				  .growth_factor = cont_->growth_factor,
				  .addr = new_addr};
}

int cont_grow(cont* cont_, size_t required_capacity)
{
	double final_capacity = cont_->capacity;
	double growth_factor = cont_->growth_factor;
	
	while (final_capacity < required_capacity)
		final_capacity *= growth_factor;
	
	size_t final_capacity_;
	if (final_capacity >= SIZE_MAX)
		final_capacity_ = required_capacity;
	else
	{
		final_capacity_ = (size_t)final_capacity;
		if (final_capacity_ < required_capacity) // sanity check
			final_capacity_ = required_capacity;
	}
		
	size_t max_capacity = cont_->max_capacity;
	
	if (max_capacity && final_capacity_ > max_capacity)
		final_capacity_ = max_capacity;
		
	size_t unit = cont_->unit;
	if (final_capacity_ > SIZE_MAX / unit)
		return SIZE_OVERFLOW;
		
	void* ptr = realloc(cont_->addr, final_capacity_*unit);
	if (!ptr)
		return REALLOC_FAILURE;
	
	cont_->addr = ptr;
	cont_->capacity = final_capacity_;
	
	return 0;
}
