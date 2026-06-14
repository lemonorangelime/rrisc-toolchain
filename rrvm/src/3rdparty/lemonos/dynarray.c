#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <3rdparty/lemonos/dynarray.h>

uint64_t round64(uint64_t x, uint64_t y) {
	return (x + (y - 1)) & ~(y - 1);
}

dynarray_t * dyna_create(uint64_t size, uint64_t block_size) {
	dynarray_t * array = malloc(sizeof(dynarray_t));
	if (!array) {
		return NULL;
	}
	array->size = size;
	array->real_size = size;
	array->block_size = block_size;
	array->array = malloc(size * block_size);
	return array;
}

dynarray_t * dyna_swap(dynarray_t * array, uint64_t size, uint64_t block_size) {
	if (!array) {
		return dyna_create(size, block_size);
	}
	void * new_array = malloc(size * block_size);
	memcpy(new_array, array->array, array->size * array->block_size);
	free(array->array);

	array->array = new_array;
	array->real_size = size;
	array->block_size = block_size;

	return array;
}

dynarray_t * dyna_grow(dynarray_t * array, uint64_t size, uint64_t block_size) {
	if (!array) {
		return dyna_create(size, block_size);
	}
	uint64_t new_size = array->size + size;
	return dyna_swap(array, new_size, block_size);
}

dynarray_t * dyna_shrink(dynarray_t * array, uint64_t size, uint64_t block_size) {
	if (!array) {
		return dyna_create(size, block_size);
	}
	if (size < 8) {
		return array; // not worth it
	}
	if (size >= array->size) {
		array->size = 0;
		free(array->array);
		return array;
	}
	uint64_t new_size = array->size + size;
	return dyna_swap(array, new_size, block_size);
}

dynarray_t * dyna_reserve(dynarray_t * array, uint64_t size, uint64_t block_size) {
	if (!array) {
		return dyna_create(size, block_size);
	}
	if (array->size >= size) {
		return array; // :shrug:
	}
	uint64_t new_size = array->size + round64(size - array->size, 8);
	dyna_grow(array, new_size, block_size);
	return array;
}

dynarray_t * dyna_append(dynarray_t * array, void * p, uint64_t block_size) {
	if (!array) {
		array = dyna_create(1, block_size);
		memcpy(array->array, p, block_size);
		return array;
	}
	if (array->size == array->real_size || array->size == 0) {
		array = dyna_grow(array, 1, block_size);
	}
	memcpy(array->array + (array->size * array->block_size), p, block_size);
	array->size++;
	return array;
}

void * dyna_step_iterator(dynarray_iterator_t * iterator) {
	dynarray_t * array = iterator->array;
	uint64_t offset = iterator->offset;
	if (offset >= (array->size * array->block_size)) {
		return NULL;
	}
	iterator->offset += array->block_size;
	return array->array + offset;
}

void dyna_iterate(dynarray_t * array, void * callback) {
	if (!callback) {
		return;
	}
	dynarray_callback_t call = callback;
	dynarray_iterator_t iterator = {array, 0};
	void ** p = NULL;
	while (p = dyna_step_iterator(&iterator)) {
		call(p);
	}
}