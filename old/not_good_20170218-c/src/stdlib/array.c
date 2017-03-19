/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-20
 * ================================
 */

#include "libzc.h"

#define ZARRAY_SPACE_LEFT(a) ((a)->size - (a)->len - 1)

zarray_t *zarray_create(int size)
{
    zarray_t *arr;

    arr = (zarray_t *) zmalloc(sizeof(zarray_t));
    if (size < 13) {
        size = 13;
    }
    arr->data = (char **)zmalloc((size + 1) * sizeof(char *));
    arr->size = size;
    arr->len = 0;

    return (arr);
}

void zarray_free(zarray_t * arr)
{
    zfree(arr->data);
    zfree(arr);
}

static void zarray_extend(zarray_t * arr)
{
    arr->size *= 2;
    arr->data = (char **)zrealloc((char *)arr->data, (arr->size + 1) * sizeof(char *));
}

void zarray_add(zarray_t * arr, void *ns)
{
    if (ZARRAY_SPACE_LEFT(arr) <= 0)
        zarray_extend(arr);
    arr->data[arr->len++] = (char *)ns;
}
