/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-12-07
 * ================================
 */

#include "zc.h"

static void *zmem_greedy_pool_malloc(zmpool_t * mp, int len);
static void *zmem_greedy_pool_calloc(zmpool_t * mp, int nmemb, int len);
static void *zmem_greedy_pool_realloc(zmpool_t * mp, const void *ptr, int len);
static void zmem_greedy_pool_free(zmpool_t * mp, const void *ptr);
static void zmem_greedy_pool_reset(zmpool_t * mp);
static void zmem_greedy_pool_free_pool(zmpool_t * mp);

static zmpool_method_t _method = {
    zmem_greedy_pool_malloc,
    zmem_greedy_pool_calloc,
    zmem_greedy_pool_realloc,
    zmem_greedy_pool_free,
    zmem_greedy_pool_reset,
    zmem_greedy_pool_free_pool
};

typedef struct zmem_greedy_pool_t zmem_greedy_pool_t;
typedef struct zmem_greedy_pool_one_t zmem_greedy_pool_one_t;
typedef struct zmem_greedy_pool_direct_t zmem_greedy_pool_direct_t;
struct zmem_greedy_pool_t {
    zmem_greedy_pool_one_t *list_head;
    zmem_greedy_pool_one_t *list_tail;
    zmem_greedy_pool_direct_t *direct_list_head;
    zmem_greedy_pool_direct_t *direct_list_tail;
    int single_buf_size;
    int once_malloc_max_size;
};

struct zmem_greedy_pool_one_t {
    zmem_greedy_pool_one_t *prev;
    zmem_greedy_pool_one_t *next;
    int used;
};

struct zmem_greedy_pool_direct_t {
    zmem_greedy_pool_direct_t *prev;
    zmem_greedy_pool_direct_t *next;
};

/* ################################################################## */
static void *zmem_greedy_pool_malloc(zmpool_t * mp, int len)
{
    zmem_greedy_pool_t *worker = (zmem_greedy_pool_t *) ((char *)mp + sizeof(zmpool_t));

    if (!worker) {
        return zmalloc(len);
    }

    if (len < 1) {
        len = 1;
    }

    if (len > worker->once_malloc_max_size) {
        zmem_greedy_pool_direct_t *d = (zmem_greedy_pool_direct_t *)zmalloc(sizeof(zmem_greedy_pool_direct_t) + len);
        ZMLINK_APPEND(worker->direct_list_head, worker->direct_list_tail, d, prev, next);
        return (((char *)d)+sizeof(zmem_greedy_pool_direct_t));
    }

    zmem_greedy_pool_one_t *one = worker->list_tail;
    if ((!one) || (len > (worker->single_buf_size - one->used))) {
        one = (zmem_greedy_pool_one_t *) zmalloc(sizeof(zmem_greedy_pool_one_t) + worker->single_buf_size + 1);
        one->used = 0;
        ZMLINK_APPEND(worker->list_head, worker->list_tail, one, prev, next);
    }
    one = worker->list_tail;
    char *r = ((char *)one) + sizeof(zmem_greedy_pool_one_t) + one->used;
    one->used += len;
    *r = 0;

    return r;
}

static void *zmem_greedy_pool_calloc(zmpool_t * mp, int nmemb, int len)
{
    void *r = zmem_greedy_pool_malloc(mp, nmemb * len);
    memset(r, 0, nmemb * len);
    return r;
}

static void *zmem_greedy_pool_realloc(zmpool_t * mp, const void *ptr, int len)
{
    zfatal("FATAL zmem_greedy_pool_realloc: unsupported");
    return 0;
}

static void zmem_greedy_pool_free(zmpool_t * mp, const void *ptr)
{
}

/* ################################################################## */
static void zmem_greedy_pool_free_pool(zmpool_t * mp)
{
    zmem_greedy_pool_reset(mp);
    zfree(mp);
}

static void zmem_greedy_pool_reset(zmpool_t * mp)
{
    zmem_greedy_pool_t *worker = (zmem_greedy_pool_t *) ((char *)mp + sizeof(zmpool_t));

    while(worker->list_head) {
        zmem_greedy_pool_one_t *one = worker->list_head;
        ZMLINK_DETACH(worker->list_head, worker->list_tail, one, prev, next);
        zfree(one);
    }

    while(worker->direct_list_head) {
        zmem_greedy_pool_direct_t *one = worker->direct_list_head;
        ZMLINK_DETACH(worker->direct_list_head, worker->direct_list_tail, one, prev, next);
        zfree(one);
    }
}

/* ################################################################## */
zmpool_t *zmpool_create_greedy_pool(int single_buf_size, int once_malloc_max_size)
{
    zmpool_t *mp;
    zmem_greedy_pool_t *worker;

    mp = (zmpool_t *) zmalloc(sizeof(zmpool_t) + sizeof(zmem_greedy_pool_t));
    mp->method=&_method;
    worker = (zmem_greedy_pool_t *) ((char *)mp + sizeof(zmpool_t));
    memset(worker, 0, sizeof(zmem_greedy_pool_t));
    worker->single_buf_size = single_buf_size;
    worker->once_malloc_max_size = once_malloc_max_size;

    return mp;
}
