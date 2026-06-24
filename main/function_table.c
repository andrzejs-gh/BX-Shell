#include <stdlib.h>
#include <string.h>
#include "function_table.h"

const ftable INVALID_FTABLE = {NULL, 0, 0};

static size_t get_hash(char* key)
{	
	unsigned char byte;
	size_t i = 0;
	size_t hash = 0;
	
	while ( (byte = key[i++]) )
		hash = (hash*P + byte) % M;

	return hash;
}

ftable ftable_new(size_t initial_capacity)
{
	ftable_entry* table = calloc(2*initial_capacity, sizeof(ftable_entry));
	if ( !table )
		return INVALID_FTABLE;
	
	return (ftable){
		.table = table,
		.table_size = 2*initial_capacity,
		.table_count = 0};
}

int ftable_free(ftable* ft)
{
	if (!ft->table)
		return TABLE_ALREADY_FREED;
	
	free(ft->table);
	*ft = INVALID_FTABLE;
	return 0;
}

ftable_entry* ftable_get(ftable* ft, char* key)
{
	size_t table_size = ft->table_size;
	size_t index = get_hash(key) % table_size;
	ftable_entry* table = ft->table;
	char* entry_key;
	
	while ( (entry_key = table[index].key) )
	{
		if ( !strcmp(entry_key, key) )
			return &table[index];
		
		if (++index == table_size)
			index = 0;
	}
	
	return NULL;
}

static int ftable_grow(ftable* ft)
{
	size_t old_table_size = ft->table_size;
	size_t new_table_size = 2*old_table_size;
	
	ftable_entry* old_table = ft->table;
	ftable_entry* new_table = calloc(new_table_size, sizeof(ftable_entry));
	if (!new_table)
		return ALLOCATION_FAILURE;
	
	char* entry_key;
	size_t new_index;
	
	while (old_table_size--)
	{
		if ( (entry_key = old_table[old_table_size].key) )
		{
			new_index = get_hash(entry_key) % new_table_size;
			while ( new_table[new_index].key )
			{
				if (++new_index == new_table_size)
					new_index = 0;
			}
			new_table[new_index] = old_table[old_table_size];
		}
	}
	free(old_table);
	ft->table = new_table;
	ft->table_size = new_table_size;
	
	return 0;
}

int ftable_set(ftable* ft, ftable_entry entry)
{
	ftable_entry* ptr = ftable_get(ft, entry.key);
	
	if (ptr)
		*ptr = entry;
	else
	{
		int ret;
		
		if (ft->table_count > LOAD_FACTOR*ft->table_size)
		{
			ret = ftable_grow(ft);
			if (ret)
				return ret;
		}	
		
		ftable_entry* table = ft->table;
		size_t table_size = ft->table_size;
		size_t index = get_hash(entry.key) % table_size;
		
		while ( table[index].key )
		{
			if (++index == table_size)
				index = 0;
		}
		table[index] = entry;
		ft->table_count++;
	}
	
	return 0;
}
