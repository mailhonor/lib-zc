/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-12-07
 * ================================
 */

#include "zcc/zcc_stdlib.h"
#include "zcc/zcc_link.h"

zcc_namespace_begin;

struct greedy_one_t;
struct greedy_direct_t;

struct greedy_mpool_t
{
    greedy_one_t *list_head;
    greedy_one_t *list_tail;
    greedy_direct_t *direct_list_head;
    greedy_direct_t *direct_list_tail;
    int single_buf_size;
    int once_malloc_max_size;
};

struct greedy_one_t
{
    greedy_one_t *prev;
    greedy_one_t *next;
    int used;
};

struct greedy_direct_t
{
    greedy_direct_t *prev;
    greedy_direct_t *next;
};

greedy_mpool::greedy_mpool()
{
}

greedy_mpool::greedy_mpool(int single_buf_size, int once_malloc_max_size)
{
    reinit(single_buf_size, once_malloc_max_size);
}

static void _greedy_mpool_free(greedy_mpool_t *m)
{
    while (m->list_head)
    {
        greedy_one_t *one = m->list_head;
        ZCC_MLINK_DETACH(m->list_head, m->list_tail, one, prev, next);
        zcc::free(one);
    }

    while (m->direct_list_head)
    {
        greedy_direct_t *one = m->direct_list_head;
        ZCC_MLINK_DETACH(m->direct_list_head, m->direct_list_tail, one, prev, next);
        zcc::free(one);
    }
    zcc::free(m);
}

greedy_mpool::~greedy_mpool()
{
    if (engine_)
    {
        _greedy_mpool_free(engine_);
    }
}

void greedy_mpool::reinit(int single_buf_size, int once_malloc_max_size)
{
    if (engine_)
    {
        _greedy_mpool_free(engine_);
        engine_ = nullptr;
    }
    engine_ = (greedy_mpool_t *)zcc::calloc(1, sizeof(greedy_mpool_t));
    memset(engine_, 0, sizeof(greedy_mpool_t));
    engine_->single_buf_size = single_buf_size;
    engine_->once_malloc_max_size = once_malloc_max_size;
}

void greedy_mpool::reset()
{
    if (!engine_)
    {
        return;
    }
    int single_buf_size = engine_->single_buf_size;
    int once_malloc_max_size = engine_->once_malloc_max_size;
    reinit(single_buf_size, once_malloc_max_size);
}

void *greedy_mpool::malloc(int64_t len)
{
    if (!engine_)
    {
        return nullptr;
    }

    if (len < 1)
    {
        len = 1;
    }

    if (len > engine_->once_malloc_max_size)
    {
        greedy_direct_t *d = (greedy_direct_t *)zcc::malloc(sizeof(greedy_direct_t) + len);
        ZCC_MLINK_APPEND(engine_->direct_list_head, engine_->direct_list_tail, d, prev, next);
        return (((char *)d) + sizeof(greedy_direct_t));
    }

    greedy_one_t *one = engine_->list_tail;
    if ((!one) || (len > (engine_->single_buf_size - one->used)))
    {
        one = (greedy_one_t *)zcc::malloc(sizeof(greedy_one_t) + engine_->single_buf_size + 1);
        one->used = 0;
        ZCC_MLINK_APPEND(engine_->list_head, engine_->list_tail, one, prev, next);
    }
    one = engine_->list_tail;
    char *r = ((char *)one) + sizeof(greedy_one_t) + one->used;
    one->used += len;
    *r = 0;

    return r;
}

void *greedy_mpool::calloc(int64_t nmemb, int64_t size)
{
    void *r = malloc(nmemb * size);
    if (r)
    {
        memset(r, 0, nmemb * size);
    }
    return r;
}

char *greedy_mpool::strdup(const char *ptr)
{
    int n = strlen(ptr);
    return (char *)memdupnull(ptr, n);
}

char *greedy_mpool::strndup(const char *ptr, int64_t n)
{
    int tn = strlen(ptr);
    if (tn > n)
    {
        tn = n;
    }
    return (char *)memdupnull(ptr, tn);
}

void *greedy_mpool::memdup(const void *ptr, int64_t n)
{
    void *r = malloc(n);
    if (r)
    {
        memcpy(r, ptr, n);
    }
    return r;
}

void *greedy_mpool::memdupnull(const void *ptr, int64_t n)
{
    void *r = malloc(n + 1);
    if (r)
    {
        memcpy(r, ptr, n);
        ((char *)r)[n] = 0;
    }
    return r;
}

zcc_namespace_end;
