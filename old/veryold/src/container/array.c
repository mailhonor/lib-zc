#include "zc.h"

#define ZARRAY_SPACE_LEFT(a) ((a)->size - (a)->len - 1)

ZARRAY *zarray_create(int size)
{
	ZARRAY *arr;

	arr = (ZARRAY *) zmalloc(sizeof(ZARRAY));
	if (size < 13) {
		size = 13;
	}
	arr->data = (char **)zmalloc((size + 1) * sizeof(char *));
	arr->size = size;
	arr->len = 0;

	return (arr);
}

void zarray_free(ZARRAY * arr, ZFN_STRUCTURE_FREE_2 free_fn, void *ctx)
{
	char **cpp;

	for (cpp = arr->data; cpp < arr->data + arr->len; cpp++) {
		if (*cpp && free_fn) {
			free_fn(*cpp, ctx);
		}
	}
	zfree(arr->data);
	zfree(arr);
}

void *zarray_free_STR(ZARRAY * arr)
{
	char **cpp;

	for (cpp = arr->data; cpp < arr->data + arr->len; cpp++) {
		if (*cpp) {
			zfree(*cpp);
		}
	}
	zfree(arr->data);
	zfree(arr);

	return (0);
}

static void zarray_extend(ZARRAY * arr)
{
	arr->size *= 2;
	arr->data = (char **)zrealloc((char *)arr->data, (arr->size + 1) * sizeof(char *));
}

void zarray_enter(ZARRAY * arr, void *ns)
{
	if (ZARRAY_SPACE_LEFT(arr) <= 0)
		zarray_extend(arr);
	arr->data[arr->len++] = (char *)ns;
}

void zarray_truncate(ZARRAY * arr, int len, ZFN_STRUCTURE_FREE_2 free_fn, void *ctx)
{
	char **cpp;

	if (len < arr->len) {
		for (cpp = arr->data + len; cpp < arr->data + arr->len; cpp++) {
			if (*cpp && free_fn) {
				free_fn(*cpp, ctx);
			}
		}
		arr->len = len;
	}
}
