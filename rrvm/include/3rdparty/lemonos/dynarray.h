#pragma once

#include <stdint.h>

typedef void (* dynarray_callback_t)(void ** p);

typedef struct {
	uint64_t size;
	uint64_t real_size;
	uint64_t block_size;
	void * array;
} dynarray_t;

typedef struct {
	dynarray_t * array;
	uint64_t offset;
	void * p;
} dynarray_iterator_t;

dynarray_t * dyna_create(uint64_t size, uint64_t block_size);
dynarray_t * dyna_swap(dynarray_t * array, uint64_t size, uint64_t block_size);
dynarray_t * dyna_grow(dynarray_t * array, uint64_t size, uint64_t block_size);
dynarray_t * dyna_shrink(dynarray_t * array, uint64_t size, uint64_t block_size);
dynarray_t * dyna_reserve(dynarray_t * array, uint64_t size, uint64_t block_size);
dynarray_t * dyna_append(dynarray_t * array, void * p, uint64_t block_size);
void * dyna_step_iterator(dynarray_iterator_t * iterator);
void dyna_iterate(dynarray_t * array, void * callback);