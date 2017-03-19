/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-30
 * ================================
 */

#include "zc.h"

void *zmem_common_pool_malloc(zmpool_t * mp, int len);
void *zmem_common_pool_realloc(zmpool_t * mp, const void *ptr, int len);
void zmem_common_pool_free(zmpool_t * mp, const void *ptr);
void zmem_common_pool_reset(zmpool_t * mp);
void zmem_common_pool_free_pool(zmpool_t * mp);

void *zmem_greedy_pool_malloc(zmpool_t * mp, int len);
void *zmem_greedy_pool_realloc(zmpool_t * mp, const void *ptr, int len);
void zmem_greedy_pool_free(zmpool_t * mp, const void *ptr);
void zmem_greedy_pool_reset(zmpool_t * mp);
void zmem_greedy_pool_free_pool(zmpool_t * mp);

zmpool_api_t zmpool_api_vector[2] = {
    {
        zmem_common_pool_malloc,
        zmem_common_pool_realloc,
        zmem_common_pool_free,
        zmem_common_pool_reset,
        zmem_common_pool_free_pool
    },
    {
        zmem_greedy_pool_malloc,
        zmem_greedy_pool_realloc,
        zmem_greedy_pool_free,
        zmem_greedy_pool_reset,
        zmem_greedy_pool_free_pool
    }
};

/* ################################################################## */
void zmpool_free_pool(zmpool_t * mp)
{
    if (mp) {
        zmpool_api_vector[mp->api_id].free_pool(mp);
    }
}

void zmpool_reset(zmpool_t * mp)
{
    if (mp) {
        zmpool_api_vector[mp->api_id].reset(mp);
    }
}

/* ################################################################## */

void *zmpool_strdup(zmpool_t * mp, const char *ptr)
{
    int len;
    char *r;

    if (!mp) {
        return zstrdup(ptr);
    }

    if ((!ptr) || (ptr == zblank_buffer)) {
        return zblank_buffer;
    }
    len = strlen(ptr);
    r = (char *)zmpool_malloc(mp, len + 1);
    if (len > 0) {
        memcpy(r, ptr, len);
    }
    r[len] = 0;

    return r;
}

void *zmpool_strndup(zmpool_t * mp, const char *ptr, int n)
{
    int len;
    char *r;

    if (!mp) {
        return strndup(ptr, n);
    }

    if ((!ptr) || (ptr == zblank_buffer) || (n < 1)) {
        return zblank_buffer;
    }
    len = strlen(ptr);
    if (n > len) {
        n = len;
    }
    r = (char *)zmpool_malloc(mp, n + 1);
    if (n > 0) {
        memcpy(r, ptr, n);
    }
    r[n] = 0;

    return r;
}

void *zmpool_memdup(zmpool_t * mp, const void *ptr, int n)
{
    char *r;

    if ((!ptr) || (n < 1)) {
        return zblank_buffer;
    }

    if (!mp) {
        return zmemdup(ptr, n);
    }

    r = (char *)zmpool_malloc(mp, n);
    memcpy(r, ptr, n);

    return r;
}

void *zmpool_memdupnull(zmpool_t * mp, const void *ptr, int n)
{
    char *r;

    if ((!ptr) || (n<1)) {
        return zblank_buffer;
    }

    if (!mp) {
        return zmemdupnull(ptr, n);
    }

    r = (char *)zmpool_malloc(mp, n + 1);
    if (n > 0) {
        memcpy(r, ptr, n);
    }
    r[n] = 0;

    return r;
}
