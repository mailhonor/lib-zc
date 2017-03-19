/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-20
 * ================================
 */

#include "zc.h"

#define ZVECTOR_SPACE_LEFT(a) ((a)->size - (a)->len - 1)

zvector_t *zvector_create_MPOOL(zmpool_t *mpool, int size)
{
    zvector_t *arr;

    arr = (zvector_t *) zmpool_malloc(mpool, sizeof(zvector_t));
    arr->mpool = mpool;
    if (size < 1) {
        size = 1;
    }
    arr->data = (char **)zmpool_malloc(mpool, (size + 1) * sizeof(char *));
    arr->size = size;
    arr->len = 0;
    arr->data[0] = 0;

    return (arr);
}

zvector_t *zvector_create(int size)
{
    return zvector_create_MPOOL(0, size);
}

void zvector_free(zvector_t * arr)
{
    zmpool_free(arr->mpool, arr->data);
    zmpool_free(arr->mpool, arr);
}

static void zvector_extend(zvector_t * arr)
{
    char **nd;
    nd = (char **)zmpool_malloc(arr->mpool, (arr->size * 2 + 1) * sizeof(char *));
    memcpy(nd, arr->data, arr->len * sizeof(char *));
    zmpool_free(arr->mpool, arr->data);
    arr->data = nd;
    arr->size *= 2;
    arr->data[arr->len] = 0;
}

void zvector_add(zvector_t * arr, void *ns)
{
    if (ZVECTOR_SPACE_LEFT(arr) <= 0)
        zvector_extend(arr);
    arr->data[arr->len++] = (char *)ns;
    arr->data[arr->len] = 0;
}

void zvector_reset(zvector_t * arr)
{
    arr->len = 0;
}

void zvector_truncate(zvector_t * arr, size_t new_len)
{
    if (new_len < arr->len) {
        arr->len = new_len;
    }
    arr->data[arr->len] = 0;
}

