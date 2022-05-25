/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-03-20
 * ================================
 */

#include "zc.h"
#include <pthread.h>

static zvector_t *___funcs_vec = 0;
static void ___before_exit(void)
{
    for (int i = ___funcs_vec->len - 1; i > -1; i -= 2) {
        void (*f)(void *) = (void *)(___funcs_vec->data[i-1]);
        void *ctx = (void *)(___funcs_vec->data[i]);
        if(f) {
            f(ctx);
        }
    }
    zvector_free(___funcs_vec);
}

static pthread_mutex_t ___before_mutex = PTHREAD_MUTEX_INITIALIZER;
void zatexit(void (*func)(void *), void *ctx)
{
    zpthread_lock(&___before_mutex);
    if (!___funcs_vec) {
        ___funcs_vec = zvector_create(32);
        atexit(___before_exit);
    }
    zvector_add(___funcs_vec, func);
    zvector_add(___funcs_vec, ctx);
    zpthread_unlock(&___before_mutex);
}

