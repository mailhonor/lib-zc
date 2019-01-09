/*
 * ================================
 * eli960@qq.com
 * www.mailhonor.com
 * 2018-12-02
 * ================================
 */

#include "zc.h"
#include <pthread.h>

static pthread_mutex_t zvar_general_pthread_mutex_buffer = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t *zvar_general_pthread_mutex = &zvar_general_pthread_mutex_buffer;
