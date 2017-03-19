/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-07
 * ================================
 */

#include "zc.h"

static int greedy_buf_size = 1024 * 1024;
static int max_malloc_size = 1024 * 100;

typedef struct zmem_greedy_pool_t zmem_greedy_pool_t;
typedef struct zmem_greedy_pool_one_t zmem_greedy_pool_one_t;
struct zmem_greedy_pool_t {
    zvector_t *list;
    zvector_t *direct_list;
    zmem_greedy_pool_one_t *current;
};
struct zmem_greedy_pool_one_t {
    char *data;
    int used;
    int size;
};

/* ################################################################## */
void *zmem_greedy_pool_malloc(zmpool_t * mp, int len)
{
    zmem_greedy_pool_t *worker = (zmem_greedy_pool_t *) ((char *)mp + sizeof(zmpool_t));
    zmem_greedy_pool_one_t *one;
    char *r;

    if (!worker) {
        return zmalloc(len);
    }

    if (len < 1) {
        return zblank_buffer;
    }

    if (len > max_malloc_size) {
        r = (char *)zmalloc(len);
        if (worker->direct_list == NULL) {
            worker->direct_list = zvector_create(1);
        }
        zvector_add(worker->direct_list, r);
        return r;
    }

    while (1) {
        one = worker->current;
        if (!one) {
            one = (zmem_greedy_pool_one_t *) zmalloc(sizeof(zmem_greedy_pool_one_t) + greedy_buf_size + 1);
            one->data = (char *)one + sizeof(zmem_greedy_pool_one_t);
            one->used = 0;
            one->size = greedy_buf_size;
            worker->current = one;
        }
        if (len > (one->size - one->used)) {
            if (worker->list == NULL) {
                worker->list = zvector_create(1);
            }
            zvector_add(worker->list, one);
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

void *zmem_greedy_pool_realloc(zmpool_t * mp, const void *ptr, int len)
{
    zmem_greedy_pool_t *worker = (zmem_greedy_pool_t *) ((char *)mp + sizeof(zmpool_t));
    zmem_greedy_pool_one_t *one, *two = 0;
    int i, olen;
    char *r;

    zfatal("greedy type mem pool does not support realloc");

    if (!ptr) {
        return zmem_greedy_pool_malloc(mp, len);
    }

    if (worker->list) {
        ZVECTOR_WALK_BEGIN(worker->list, one) {
            if ((char *)ptr >= (char *)(one->data)) {
                if ((char *)ptr <= ((char *)(one->data) + greedy_buf_size)) {
                    two = one;
                    break;
                }
            }
        }
        ZVECTOR_WALK_END;
    }
    one = worker->current;
    if ((char *)ptr >= (char *)(one->data)) {
        if ((char *)ptr <= ((char *)(one->data) + greedy_buf_size)) {
            two = one;
        }
    }

    if (two) {
        olen = (char *)(two->data) + greedy_buf_size - (char *)ptr;
        r = (char *)zmem_greedy_pool_malloc(mp, len);
        if (olen >= len) {
            olen = len;
        }
        memmove(r, ptr, olen);
        return r;
    }
    if (worker->direct_list) {
        for (i = 0; i < worker->direct_list->len; i++) {
            if ((char *)(worker->direct_list->data[i]) == (char *)ptr) {
                r = (char *)zrealloc(ptr, len);
                worker->direct_list->data[i] = r;
                return r;
            }
        }
    }

    zfatal("zmem_greedy_pool_realloc: unknown ptr");

    return 0;
}

void zmem_greedy_pool_free(zmpool_t * mp, const void *ptr)
{
    zmem_greedy_pool_t *worker = (zmem_greedy_pool_t *) ((char *)mp + sizeof(zmpool_t));
    zmem_greedy_pool_one_t *one;
    int i;

    if ((!ptr) || (ptr == zblank_buffer)) {
        return;
    }
    if (worker->list) {
        ZVECTOR_WALK_BEGIN(worker->list, one) {
            if ((char *)ptr >= (char *)(one->data)) {
                if ((char *)ptr <= ((char *)(one->data) + greedy_buf_size)) {
                    return;
                }
            }
        }
        ZVECTOR_WALK_END;
    }
    one = worker->current;
    if ((char *)ptr >= (char *)(one->data)) {
        if ((char *)ptr <= ((char *)(one->data) + greedy_buf_size)) {
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
void zmem_greedy_pool_free_pool(zmpool_t * mp)
{
    zmem_greedy_pool_t *worker = (zmem_greedy_pool_t *) ((char *)mp + sizeof(zmpool_t));
    zmem_greedy_pool_one_t *one;

    if (worker->current) {
        zfree(worker->current);
    }

    if (worker->list) {
        ZVECTOR_WALK_BEGIN(worker->list, one) {
            zfree(one);
        }
        ZVECTOR_WALK_END;
        zvector_free(worker->list);
    }

    if (worker->direct_list) {
        ZVECTOR_WALK_BEGIN(worker->direct_list, one) {
            zfree(one);
        }
        ZVECTOR_WALK_END;
        zvector_free(worker->direct_list);
    }

    zfree(mp);
}

void zmem_greedy_pool_reset(zmpool_t * mp)
{
    zmem_greedy_pool_t *worker = (zmem_greedy_pool_t *) ((char *)mp + sizeof(zmpool_t));
    zmem_greedy_pool_one_t *one;

    if (worker->current) {
        zfree(worker->current);
    }
    worker->current = 0;

    if (worker->list) {
        ZVECTOR_WALK_BEGIN(worker->list, one) {
            zfree(one);
        }
        ZVECTOR_WALK_END;
        zvector_free(worker->list);
    }
    worker->list = 0;

    if (worker->direct_list) {
        ZVECTOR_WALK_BEGIN(worker->direct_list, one) {
            zfree(one);
        }
        ZVECTOR_WALK_END;
        zvector_free(worker->direct_list);
    }
    worker->direct_list = 0;
}

/* ################################################################## */
zmpool_t *zmpool_create_greedy_pool(void)
{
    zmpool_t *mp;
    zmem_greedy_pool_t *worker;

    mp = (zmpool_t *) zmalloc(sizeof(zmpool_t) + sizeof(zmem_greedy_pool_t));
    mp->api_id=1;
    worker = (zmem_greedy_pool_t *) ((char *)mp + sizeof(zmpool_t));
    memset(worker, 0, sizeof(zmem_greedy_pool_t));

    return mp;
}
