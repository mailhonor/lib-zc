/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-07
 * ================================
 */

#include "libzc.h"

static int grow_buf_size = 1024 * 1024;
static int max_malloc_size = 1024 * 100;

typedef struct zmem_grow_pool_t zmem_grow_pool_t;
typedef struct zmem_grow_pool_one_t zmem_grow_pool_one_t;
struct zmem_grow_pool_t {
    zarray_t *list;
    zarray_t *direct_list;
    zmem_grow_pool_one_t *current;
};
struct zmem_grow_pool_one_t {
    char *data;
    int used;
    int size;
};

/* ################################################################## */
static void *zmem_grow_pool_malloc(zmpool_t * mp, int len)
{
    zmem_grow_pool_t *worker = (zmem_grow_pool_t *) (mp->worker);
    zmem_grow_pool_one_t *one;
    char *r;

    if (!worker) {
        return zmalloc(len);
    }

    if (len < 1) {
        len = 1;
    }

    if (len > max_malloc_size) {
        r = (char *)zmalloc(len);
        if (worker->direct_list == NULL) {
            worker->direct_list = zarray_create(1);
        }
        zarray_add(worker->direct_list, r);
        return r;
    }

    while (1) {
        one = worker->current;
        if (!one) {
            one = (zmem_grow_pool_one_t *) zmalloc(sizeof(zmem_grow_pool_one_t) + grow_buf_size + 1);
            one->data = (char *)one + sizeof(zmem_grow_pool_one_t);
            one->used = 0;
            one->size = grow_buf_size;
            worker->current = one;
        }
        if (len > (one->size - one->used)) {
            if (worker->list == NULL) {
                worker->list = zarray_create(1);
            }
            zarray_add(worker->list, one);
            worker->current = 0;
            continue;
        }
        break;
    }
    r = one->data + one->used;
    one->used += len;

    *r = 0;

    return r;
}

static void *zmem_grow_pool_realloc(zmpool_t * mp, void *ptr, int len)
{
    zmem_grow_pool_t *worker = (zmem_grow_pool_t *) (mp->worker);
    zmem_grow_pool_one_t *one, *two = 0;
    int i, olen;
    char *r;

    zfatal("grow type mem pool does not support realloc");

    if (!ptr) {
        return zmem_grow_pool_malloc(mp, len);
    }

    if (worker->list) {
        ZARRAY_WALK_BEGIN(worker->list, one) {
            if ((char *)ptr >= (char *)(one->data)) {
                if ((char *)ptr <= ((char *)(one->data) + grow_buf_size)) {
                    two = one;
                    break;
                }
            }
        }
        ZARRAY_WALK_END;
    }
    one = worker->current;
    if ((char *)ptr >= (char *)(one->data)) {
        if ((char *)ptr <= ((char *)(one->data) + grow_buf_size)) {
            two = one;
        }
    }

    if (two) {
        olen = (char *)(two->data) + grow_buf_size - (char *)ptr;
        r = zmem_grow_pool_malloc(mp, len);
        if (olen >= len) {
            olen = len;
        }
        memmove(r, ptr, olen);
        return r;
    }
    if (worker->direct_list) {
        for (i = 0; i < worker->direct_list->len; i++) {
            if ((char *)(worker->direct_list->data[i]) == (char *)ptr) {
                r = zrealloc(ptr, len);
                worker->direct_list->data[i] = r;
                return r;
            }
        }
    }

    zfatal("zmem_grow_pool_realloc: unknown ptr");

    return 0;
}

void zmem_grow_pool_free(zmpool_t * mp, void *ptr)
{
    zmem_grow_pool_t *worker = (zmem_grow_pool_t *) (mp->worker);
    zmem_grow_pool_one_t *one;
    int i;

    if (worker->list) {
        ZARRAY_WALK_BEGIN(worker->list, one) {
            if ((char *)ptr >= (char *)(one->data)) {
                if ((char *)ptr <= ((char *)(one->data) + grow_buf_size)) {
                    return;
                }
            }
        }
        ZARRAY_WALK_END;
    }
    one = worker->current;
    if ((char *)ptr >= (char *)(one->data)) {
        if ((char *)ptr <= ((char *)(one->data) + grow_buf_size)) {
            return;
        }
    }

    if (worker->direct_list) {
        for (i = 0; i < worker->direct_list->len; i++) {
            if ((char *)(worker->direct_list->data[i]) == (char *)ptr) {
                zfree(ptr);
                worker->direct_list->data[i] = 0;
                return;
            }
        }
    }
}

/* ################################################################## */
void zmem_grow_pool_free_pool(zmpool_t * mp)
{
    zmem_grow_pool_t *worker = (zmem_grow_pool_t *) (mp->worker);
    zmem_grow_pool_one_t *one;

    if (worker->current) {
        zfree(worker->current);
    }

    if (worker->list) {
        ZARRAY_WALK_BEGIN(worker->list, one) {
            zfree(one);
        }
        ZARRAY_WALK_END;
        zarray_free(worker->list, 0, 0);
    }

    if (worker->direct_list) {
        ZARRAY_WALK_BEGIN(worker->direct_list, one) {
            zfree(one);
        }
        ZARRAY_WALK_END;
        zarray_free(worker->direct_list, 0, 0);
    }

    zfree(mp);
}

void zmem_grow_pool_reset(zmpool_t * mp)
{
    zmem_grow_pool_t *worker = (zmem_grow_pool_t *) (mp->worker);
    zmem_grow_pool_one_t *one;

    if (worker->current) {
        zfree(worker->current);
    }
    worker->current = 0;

    if (worker->list) {
        ZARRAY_WALK_BEGIN(worker->list, one) {
            zfree(one);
        }
        ZARRAY_WALK_END;
        zarray_free(worker->list, 0, 0);
    }
    worker->list = 0;

    if (worker->direct_list) {
        ZARRAY_WALK_BEGIN(worker->direct_list, one) {
            zfree(one);
        }
        ZARRAY_WALK_END;
        zarray_free(worker->direct_list, 0, 0);
    }
    worker->direct_list = 0;
}

/* ################################################################## */
zmpool_t *zmpool_create_grow_pool(void)
{
    zmpool_t *mp;
    zmem_grow_pool_t *worker;

    mp = (zmpool_t *) zmalloc(sizeof(zmpool_t) + sizeof(zmem_grow_pool_t));
    worker = (zmem_grow_pool_t *) ((char *)mp + sizeof(zmpool_t));
    mp->worker = worker;
    memset(worker, 0, sizeof(zmem_grow_pool_t));

    mp->malloc = zmem_grow_pool_malloc;
    mp->realloc = zmem_grow_pool_realloc;
    mp->free = zmem_grow_pool_free;
    mp->free_pool = zmem_grow_pool_free_pool;
    mp->reset = zmem_grow_pool_reset;

    return mp;
}
