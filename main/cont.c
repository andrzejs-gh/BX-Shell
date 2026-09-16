#include "cont.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

const cont INVALID_CONT = (cont){0};

cont cont_new(size_t capacity, size_t unit, size_t alignment)
{
    if ( !capacity || !unit || !alignment || capacity > SIZE_MAX / unit )
		return INVALID_CONT;

	unsigned char* addr;

	if ( alignment <= _Alignof(max_align_t) )
	{
		addr = malloc(capacity*unit);
	}
	else // overaligned objects
	{
		if ( unit % alignment != 0 )
			return INVALID_CONT;

		addr = aligned_alloc(alignment, capacity*unit);
	}
	if ( !addr ) return INVALID_CONT;

    cont cnt = {
        .unit = unit,
		.alignment = alignment,
        .count = 0,
        .capacity = capacity,
        .max_capacity = cont_NO_LIMIT,
        .growth_factor = cont_GF,
        .addr = addr,
    };

    return cnt;
}

int cont_is_valid(cont* cnt)
{
	if ( !cnt )
		return 0;

	size_t capacity = cnt->capacity;
	double growth_factor = cnt->growth_factor;

	if ( !cnt->unit || !cnt->alignment || !capacity || cnt->count > capacity )
		return 0;

	if ( cnt->max_capacity && (capacity > cnt->max_capacity) )
		return 0;

	if (
		   !(
				(growth_factor > cont_GF_LBOUND) &&
				(growth_factor < cont_GF_UBOUND)
			) || !isfinite(growth_factor)

	   ) return 0;

	if ( !cnt->addr )
		return 0;

	return 1;
}

int cont_set_count(cont* cnt, size_t count)
{
	if ( !cnt )
		return CONT_IS_NULL;

	if ( (cnt->max_capacity != cont_NO_LIMIT) && (count > cnt->max_capacity) )
		return MAX_CAPACITY_EXCEEDED;

	if ( count > cnt->capacity )
	{
		int ret = cont_grow(cnt, count);
		if ( ret ) return ret;
	}

	cnt->count = count;

	return 0;
}

int cont_set_capacity(cont* cnt, size_t capacity)
{
	if ( !cnt )
		return CONT_IS_NULL;

	if ( capacity == cnt->capacity )
		return 0;

	if ( !capacity )
		return NULL_CAPACITY;

	size_t unit = cnt->unit;

	if ( capacity > SIZE_MAX / unit )
		return SIZE_OVERFLOW;

	size_t max_capacity = cnt->max_capacity;

	if ( (max_capacity != cont_NO_LIMIT) && (capacity > max_capacity) )
		return MAX_CAPACITY_EXCEEDED;

	void* ptr;
	if ( cnt->alignment <= _Alignof(max_align_t) )
	{
		ptr = realloc(cnt->addr, capacity*unit);
		if ( !ptr )
			return REALLOC_FAILURE;
	}
	else // cont contains overaligned objects
	{
		ptr = aligned_alloc(cnt->alignment, capacity*unit);
		if ( !ptr )
			return ALIGNED_ALLOC_FAILURE;

		size_t size_to_copy = (capacity > cnt->count) ? cnt->count : capacity;
			   size_to_copy *= unit;

		memcpy(ptr, cnt->addr, size_to_copy);
		free(cnt->addr);
	}

	if (cnt->count > capacity)
		cnt->count = capacity;

	cnt->capacity = capacity;
	cnt->addr = ptr;

	return 0;
}

int cont_set_max_capacity(cont* cnt, size_t max_size)
{
	if ( !cnt )
		return CONT_IS_NULL;

	size_t unit = cnt->unit;
	if ( max_size > SIZE_MAX / unit )
		return SIZE_OVERFLOW;

	if ( max_size == cont_NO_LIMIT )
	{
		cnt->max_capacity = cont_NO_LIMIT;
	}
	else if ( cnt->count > max_size || cnt->capacity > max_size )
	{
		void* ptr;
		if ( cnt->alignment <= _Alignof(max_align_t) )
		{
			ptr = realloc(cnt->addr, max_size*unit);
			if ( !ptr )
				return REALLOC_FAILURE;
		}
		else // cont contains overaligned objects
		{
			ptr = aligned_alloc(cnt->alignment, max_size*unit);
			if ( !ptr )
				return ALIGNED_ALLOC_FAILURE;

			size_t size_to_copy = (max_size > cnt->count) ? cnt->count : max_size;
				   size_to_copy *= unit;

			memcpy(ptr, cnt->addr, size_to_copy);
			free(cnt->addr);
		}

		cnt->addr = ptr;
		cnt->max_capacity = max_size;
		cnt->capacity = max_size;

		if ( cnt->count > max_size )
			cnt->count = max_size;
	}
	else
	{
		cnt->max_capacity = max_size;
	}

	return 0;
}

int cont_set_growth_factor(cont* cnt, double growth_factor)
{
	if ( !cnt )
		return CONT_IS_NULL;

	if (
		   !(
				(growth_factor > cont_GF_LBOUND) &&
				(growth_factor < cont_GF_UBOUND)

			) || !isfinite(growth_factor)

	   ) return INVALID_GROWTH_FACTOR_VALUE;

	cnt->growth_factor = growth_factor;

	return 0;
}

int cont_lock(cont* cnt)
{
	if ( !cnt )
		return CONT_IS_NULL;
	if ( !cnt->count )
		return EMPTY_CONT;

	return cont_set_max_capacity(cnt, cnt->count);
}

void* cont_get(cont* cnt, size_t index)
{
	if ( !cnt )
		return NULL;

	if ( index >= cnt->count )
		return NULL;
	else
		return cnt->addr + (index*(cnt->unit));
}

int cont_set(cont* cnt, size_t index, void* item)
{
	if ( !cnt )
		return CONT_IS_NULL;

	if ( index >= cnt->count )
		return INVALID_INDEX;
	if ( !item )
		return NULL_ITEM_POINTER;

	size_t unit = cnt->unit;

	if ( item == cnt->addr+(index*unit) ) // if item points to itself
		return 0;

	memcpy(cnt->addr+(index*unit), item, unit);

	return 0;
}

int cont_cv(cont* cnt, size_t index, void* buffer, size_t n)
{
	if ( !cnt )
		return CONT_IS_NULL;

	if ( !buffer )
		return NULL_ARRAY_POINTER;

	size_t count = cnt->count;

	if ( index >= count )
		return INVALID_INDEX;

	if ( n == cont_ALL )
		n = count - index;
	else if (n > count - index)
		return INVALID_RANGE;

	size_t unit = cnt->unit;

	uintptr_t buff_begin = (uintptr_t)buffer;
	uintptr_t buff_end = buff_begin + n*unit;
	uintptr_t cont_begin = (uintptr_t)cnt->addr;
	uintptr_t cont_end = cont_begin + cnt->capacity*unit;

	if ( buff_begin < cont_end && cont_begin < buff_end )
		return BUFFER_OVERLAP;

	memcpy(buffer, (cnt->addr+index*unit), n*unit);

	return 0;
}

void* cont_pop(cont* cnt)
{
	if ( !cnt )
		return NULL;

	size_t count = cnt->count;
	if ( !count )
		return NULL;

	cnt->count--;
	return cnt->addr + (count-1)*cnt->unit;
}

int cont_mkroom(cont* cnt, size_t index)
{
	if ( !cnt )
		return CONT_IS_NULL;

	size_t count = cnt->count;

	if ( index >= count )
		return INVALID_INDEX;

	if ( (cnt->max_capacity != cont_NO_LIMIT) && (count == cnt->max_capacity) )
		return MAX_CAPACITY_EXCEEDED;

	if ( count == cnt->capacity )
	{
		int ret = cont_grow(cnt, cnt->capacity+1);
		if ( ret ) return ret;
	}

	size_t unit = cnt->unit;
	unsigned char* addr = cnt->addr;

	memmove(addr+((index+1)*unit), addr+(index*unit), (count-index)*unit);

	return 0;
}

int cont_push(cont* cnt, void* item)
{
	if ( !cnt )
		return CONT_IS_NULL;

	if ( !item )
		return NULL_ITEM_POINTER;

	size_t count = cnt->count;
	size_t max_capacity = cnt->max_capacity;

	if ( (max_capacity != cont_NO_LIMIT) && (count == max_capacity) )
		return MAX_CAPACITY_EXCEEDED;

	size_t unit = cnt->unit;

	uintptr_t arr_begin = (uintptr_t)item;
	uintptr_t arr_end = arr_begin + unit;
	uintptr_t cont_begin = (uintptr_t)cnt->addr;
	uintptr_t cont_end = cont_begin + cnt->capacity*unit;

	if ( arr_begin < cont_end && cont_begin < arr_end )
		return BUFFER_OVERLAP;

	if ( count == cnt->capacity )
	{
		int ret = cont_grow(cnt, count+1);
		if ( ret ) // cont_grow returns error code
			return ret;
	}

	memcpy(cnt->addr + count*unit, item, unit);
	cnt->count++;

	return 0;
}

int cont_push_front(cont* cnt, void* item)
{
	if ( !cnt )
		return CONT_IS_NULL;

	if ( !item )
		return NULL_ITEM_POINTER;

	size_t count = cnt->count;
	size_t max_capacity = cnt->max_capacity;

	if ( (max_capacity != cont_NO_LIMIT) && (count == max_capacity) )
		return MAX_CAPACITY_EXCEEDED;

	size_t unit = cnt->unit;

	uintptr_t item_begin = (uintptr_t)item;
	uintptr_t item_end = item_begin + unit;
	uintptr_t cont_begin = (uintptr_t)cnt->addr;
	uintptr_t cont_end = cont_begin + cnt->capacity*unit;

	if ( item_begin < cont_end && cont_begin < item_end )
		return BUFFER_OVERLAP;

	if ( count == cnt->capacity ) // if there is no space
	{
		int ret = cont_grow(cnt, count+1);
		if ( ret ) // cont_grow returns error code
			return ret;
	}

	unsigned char* addr = cnt->addr;

	memmove(addr+unit, addr, count*unit);
	memcpy(addr, item, unit);

	cnt->count++;

	return 0;
}

int cont_write(cont* cnt, size_t index, void* arr, size_t num_of_items)
{
	if ( !cnt )
		return CONT_IS_NULL;

	if ( !arr )
        return NULL_ARRAY_POINTER;
    if ( !num_of_items )
		return NULL_ELEMENT_COUNT;

	size_t count = cnt->count;

	if ( index > count )
		return INVALID_INDEX;
	if ( index > SIZE_MAX - num_of_items )
		return SIZE_OVERFLOW;

	size_t unit = cnt->unit;

	if ( num_of_items > SIZE_MAX / unit )
		return SIZE_OVERFLOW;

	size_t size_to_copy = num_of_items*unit;

	uintptr_t arr_begin = (uintptr_t)arr;
	uintptr_t arr_end = arr_begin + size_to_copy;
	uintptr_t cont_begin = (uintptr_t)cnt->addr;
	uintptr_t cont_end = cont_begin + cnt->capacity*unit;

	if ( arr_begin < cont_end && cont_begin < arr_end )
		return BUFFER_OVERLAP;

	size_t last_indx_plus_one = index + num_of_items;

	if ( last_indx_plus_one > cnt->capacity )
	{
		size_t max_capacity = cnt->max_capacity;
		if ( (max_capacity != cont_NO_LIMIT) && (last_indx_plus_one > max_capacity) )
			return MAX_CAPACITY_EXCEEDED;

		int ret = cont_grow(cnt, last_indx_plus_one);
		if ( ret ) // cont_grow returns an error code
			return ret;
		// [|0, 1, 2, 3|, (4, 5), 6, 7, 8, ...]
		cnt->count = last_indx_plus_one;
	}
	else if ( last_indx_plus_one > count )
	{
		cnt->count = last_indx_plus_one;
	}

	memcpy(cnt->addr+(index*unit), arr, size_to_copy);

    return 0;
}

int cont_insert(cont* cnt, size_t index, void* item)
{
	if ( !cnt )
		return CONT_IS_NULL;

	size_t count = cnt->count;

	if ( !item )
		return NULL_ITEM_POINTER;
	if ( index > count )
		return INVALID_INDEX;

	if ( (cnt->max_capacity != cont_NO_LIMIT) && (count == cnt->max_capacity) )
		return MAX_CAPACITY_EXCEEDED;

	size_t unit = cnt->unit;

	uintptr_t item_begin = (uintptr_t)item;
	uintptr_t item_end = item_begin + unit;
	uintptr_t cont_begin = (uintptr_t)cnt->addr;
	uintptr_t cont_end = cont_begin + cnt->capacity*unit;

	if ( item_begin < cont_end && cont_begin < item_end )
		return BUFFER_OVERLAP;

	if ( count == cnt->capacity )
	{
		int ret = cont_grow(cnt, count+1);
		if ( ret )  // cont_grow returns and error code
			return ret;
	}

	unsigned char* position = cnt->addr + index*unit;

	memmove(position+unit, position, (count - index)*unit);
	memcpy(position, item, unit);
	cnt->count++;

	return 0;
}

int cont_insert_range(cont* cnt, size_t index, void* arr, size_t num_of_items)
{
	if ( !cnt )
		return CONT_IS_NULL;

	if ( !arr )
        return NULL_ARRAY_POINTER;
    if ( !num_of_items )
		return NULL_ELEMENT_COUNT;

	size_t count = cnt->count;

	if ( index > count )
		return INVALID_INDEX;

	size_t unit = cnt->unit;

	if ( count > SIZE_MAX - num_of_items)
		return SIZE_OVERFLOW;

	size_t max_capacity = cnt->max_capacity;

	if ( (max_capacity != cont_NO_LIMIT) && (count + num_of_items > max_capacity) )
		return MAX_CAPACITY_EXCEEDED;
	if ( num_of_items > SIZE_MAX / unit )
		return SIZE_OVERFLOW;

	size_t inserted_size = num_of_items*unit;

	uintptr_t arr_begin = (uintptr_t)arr;
	uintptr_t arr_end = arr_begin + inserted_size;
	uintptr_t cont_begin = (uintptr_t)cnt->addr;
	uintptr_t cont_end = cont_begin + cnt->capacity*unit;

	if ( arr_begin < cont_end && cont_begin < arr_end )
		return BUFFER_OVERLAP;

	size_t new_count = count + num_of_items;

	if ( new_count > cnt->capacity )
	{
		int ret = cont_grow(cnt, new_count);
		if ( ret ) // cont_grow returns an error code
			return ret;
	}

	unsigned char* position = cnt->addr + index*unit;

	memmove(position+inserted_size, position, (count-index)*unit);
	memcpy(position, arr, inserted_size);
	cnt->count += num_of_items;

	return 0;
}

int cont_append(cont* cnt, void* arr, size_t n)
{
	return cont_write(cnt, cnt->count, arr, n);
}

int cont_prepend(cont* cnt, void* arr, size_t n)
{
	return cont_insert_range(cnt, 0, arr, n);
}

int cont_set_space(cont* cnt, size_t n)
{
	if ( !cnt )
		return CONT_IS_NULL;

	size_t count = cnt->count;
	if ( !count )
		return EMPTY_CONT;

	if ( count > SIZE_MAX - n )
		return SIZE_OVERFLOW;

	size_t new_capacity = count + n;
	size_t max_capacity = cnt->max_capacity;

	if ( (max_capacity != cont_NO_LIMIT) && (new_capacity > max_capacity) )
		return MAX_CAPACITY_EXCEEDED;

	size_t unit = cnt->unit;

	if ( new_capacity > SIZE_MAX / unit )
		return SIZE_OVERFLOW;

	void* ptr;
	if ( cnt->alignment <= _Alignof(max_align_t) )
	{
		ptr = realloc(cnt->addr, new_capacity*unit);
		if ( !ptr )
			return REALLOC_FAILURE;
	}
	else
	{
		ptr = aligned_alloc(cnt->alignment, new_capacity*unit);
		if ( !ptr )
			return ALIGNED_ALLOC_FAILURE;

		memcpy(ptr, cnt->addr, count*unit);
		free(cnt->addr);
	}

	cnt->capacity = new_capacity;
	cnt->addr = ptr;

	return 0;
}

int cont_shrink(cont* cnt)
{
	return cont_set_space(cnt, 0);
}

int cont_remove(cont* cnt, size_t index)
{
	if ( !cnt )
		return CONT_IS_NULL;

	size_t count = cnt->count;
	if ( index >= count )
		return INVALID_INDEX;

	size_t unit = cnt->unit;
	unsigned char* begin = cnt->addr + index*unit;

	memmove(begin, begin+unit, (count-1-index)*unit);
	cnt->count--;

	return 0;
}

int cont_remove_range(cont* cnt, size_t index, size_t n)
{
	if ( !cnt )
		return CONT_IS_NULL;

	size_t count = cnt->count;

	if ( index >= count )
		return INVALID_INDEX;

	if ( n == cont_ALL )
		n = count - index;
	else if ( n > count - index )
		return INVALID_RANGE;

	size_t unit = cnt->unit;
	unsigned char* begin = cnt->addr + index*unit;
	unsigned char* end = begin + n*unit;

	memmove(begin, end, (count-index-n)*unit);
	cnt->count -= n;

	return 0;
}

int cont_reverse(cont* cnt)
{
	if ( !cnt )
		return CONT_IS_NULL;

	size_t count = cnt->count;

	if ( !count )
		return EMPTY_CONT;
	if ( count == 1 )
		return 0;

	size_t unit = cnt->unit;

	unsigned char* front = cnt->addr;
	unsigned char* back = cnt->addr + (count-1)*unit;
	unsigned char stack_buffer[cont_UNIT_BUFF_SIZE];
	unsigned char* buffer;

	if ( unit <= cont_UNIT_BUFF_SIZE )
		buffer = stack_buffer;
	else
	{
		buffer = malloc(unit);
		if ( !buffer )
			return MALLOC_FAILURE;
	}

	while ( back > front )
	{
		memcpy(buffer, front, unit);
		memcpy(front, back, unit);
		memcpy(back, buffer, unit);
		front += unit;
		back -= unit;
	}

	if ( buffer != stack_buffer )
		free(buffer);

	return 0;
}

int cont_clear(cont* cnt)
{
	if ( !cnt )
		return CONT_IS_NULL;

	cnt->count = 0;
	return 0;
}

int cont_set_blank(cont* cnt, size_t position, size_t n)
{
	if ( !cnt )
		return CONT_IS_NULL;

	size_t capacity = cnt->capacity;

	if ( position >= capacity )
		return INVALID_INDEX;

	if ( n == cont_ALL )
		n = capacity - position;
	else if ( n > capacity - position )
		return INVALID_RANGE;

	size_t unit = cnt->unit;

	memset((cnt->addr + position*unit), 0, n*unit);
	return 0;
}

int cont_collapse(cont* cnt)
{
	if ( !cnt )
		return CONT_IS_NULL;

	int ret = cont_set_capacity(cnt, 1);
	if ( ret )
		return ret;

	cnt->count = 0;
	return 0;
}

int cont_free(cont* cnt)
{
	if ( !cnt )
		return CONT_IS_NULL;

	if ( !cnt->addr )
		return CONT_ALREADY_FREED;

	free(cnt->addr);

	*cnt = INVALID_CONT;

	return 0;
}

cont cont_clone(cont* cnt)
{
	if ( !cnt )
		return INVALID_CONT;

	size_t unit = cnt->unit;

	void* cloned_cont_addr;
	if ( cnt->alignment <= _Alignof(max_align_t) )
	{
		cloned_cont_addr = malloc(cnt->capacity*unit);
	}
	else // cont contains overaligned objects
	{
		cloned_cont_addr = aligned_alloc(cnt->alignment, cnt->capacity*unit);
	}
	if ( !cloned_cont_addr )
		return INVALID_CONT;

	memcpy(cloned_cont_addr, cnt->addr, cnt->count*unit);

	cont clone = *cnt;
	clone.addr = cloned_cont_addr;

	return clone;
}

int cont_extend(cont* cnt, cont* cnt2)
{
	if ( !cnt || !cnt2 )
		return CONT_IS_NULL;

	if ( cnt == cnt2 )
		return SAME_CONT;
	if ( cnt->unit != cnt2->unit )
		return UNIT_MISMATCH;
	if ( !cnt2->count )
		return 0;

	return cont_append(cnt, cnt2->addr, cnt2->count);
}

cont cont_split(cont* cnt, size_t index)
{
	if ( !cnt )
		return INVALID_CONT;

	size_t count = cnt->count;

	if ( !index || index >= count )
		return INVALID_CONT;

	size_t unit = cnt->unit;

	size_t new_cont_count = count - index;
	size_t new_cont_size = new_cont_count*unit;

	void* new_addr;
	if ( cnt->alignment <= _Alignof(max_align_t) )
	{
		new_addr = malloc(new_cont_size);
	}
	else // cont contains overaligned objects
	{
		new_addr = aligned_alloc(cnt->alignment, new_cont_size);
	}
	if ( !new_addr )
		return INVALID_CONT;

	memcpy(new_addr, cnt->addr+(index*unit), new_cont_size);
	if ( cont_set_capacity(cnt, index) ) // if cnt trimming fails
	{
		free(new_addr);
		return INVALID_CONT;
	}

	cont new_cont = *cnt;
	new_cont.count = new_cont_count;
	new_cont.capacity = new_cont_count;
	new_cont.addr = new_addr;

	return new_cont;
}

cont cont_sub(cont* cnt, size_t index, size_t n_elements)
{
	if ( !cnt )
		return INVALID_CONT;

	size_t count = cnt->count;

	if ( index >= count )
		return INVALID_CONT;

	if ( n_elements == cont_ALL )
		n_elements = count - index;
	else if ( n_elements > count - index )
		return INVALID_CONT;

	size_t unit = cnt->unit;
	size_t size = n_elements*unit;

	void* sub_cont_addr;
	if ( cnt->alignment <= _Alignof(max_align_t) )
	{
		sub_cont_addr = malloc(size);
	}
	else	// cont contains overaligned objects
	{
		sub_cont_addr = aligned_alloc(cnt->alignment, size);
	}
	if ( !sub_cont_addr )
		return INVALID_CONT;

	memcpy(sub_cont_addr, cnt->addr+(index*unit), size);

	cont sub_cont = *cnt;
	sub_cont.count = n_elements;
	sub_cont.capacity = n_elements;
	sub_cont.max_capacity = cont_NO_LIMIT;
	sub_cont.addr = sub_cont_addr;

	return sub_cont;
}

int cont_grow(cont* cnt, size_t required_capacity)
{
	if ( !cnt )
		return CONT_IS_NULL;
	if ( required_capacity <= cnt->capacity )
		return INVALID_REQUIRED_CAPACITY;

	double final_capacity = cnt->capacity;
	double growth_factor = cnt->growth_factor;

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

	size_t max_capacity = cnt->max_capacity;

	if ( (max_capacity != cont_NO_LIMIT) && (final_capacity_ > max_capacity) )
		final_capacity_ = max_capacity;

	size_t unit = cnt->unit;
	if ( final_capacity_ > SIZE_MAX / unit )
		return SIZE_OVERFLOW;

	void* ptr;
	if ( cnt->alignment <= _Alignof(max_align_t) )
	{
		ptr = realloc(cnt->addr, final_capacity_*unit);
		if ( !ptr ) return REALLOC_FAILURE;
	}
	else // cont cantains overaligned objects
	{
		ptr = aligned_alloc(cnt->alignment, final_capacity_*unit);
		if ( !ptr ) return ALIGNED_ALLOC_FAILURE;

		memcpy(ptr, cnt->addr, cnt->count*unit);
		free(cnt->addr);
	}

	cnt->addr = ptr;
	cnt->capacity = final_capacity_;

	return 0;
}

int cont_ensure(cont* cnt, size_t space)
{
	if ( !cnt )
		return CONT_IS_NULL;

	if ( (cnt->capacity - cnt->count) < space )
	{
		if ( space > SIZE_MAX - cnt->count )
			return SIZE_OVERFLOW;

		int ret = cont_grow(cnt, cnt->count+space);
		if ( ret )
			return ret;
	}

	return 0;
}
