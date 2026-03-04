/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-05-30
 * ================================
 */

#include <mutex>
#ifdef _WIN64
#else //
#include <pthread.h>
#endif //
#include "zcc/zcc_stdlib.h"

zcc_namespace_begin;

static std::mutex global_low_level_mutex;

void global_low_level_mutex_lock()
{
    global_low_level_mutex.lock();
}

void global_low_level_mutex_unlock()
{
    global_low_level_mutex.unlock();
}

struct os_rwlocker_t
{
#ifdef _WIN64
    SRWLOCK srwlock; // Windows Slim Reader-Writer Lock
    bool rw_mode{false};
#else
    pthread_rwlock_t rwlock;
#endif
};

rwlocker::rwlocker()
{
    rwlocker_ = (os_rwlocker_t *)calloc(1, sizeof(os_rwlocker_t));
#ifdef _WIN64
    InitializeSRWLock(&rwlocker_->srwlock);
#else
    pthread_rwlock_init(&rwlocker_->rwlock, nullptr);
#endif
}

rwlocker::~rwlocker()
{
#ifdef _WIN64
#else
    pthread_rwlock_destroy(&rwlocker_->rwlock);
#endif
    free(rwlocker_);
}

void rwlocker::read_lock()
{
#ifdef _WIN64
    AcquireSRWLockShared(&rwlocker_->srwlock);
    rwlocker_->rw_mode = false;
#else
    pthread_rwlock_rdlock(&rwlocker_->rwlock);
#endif
}

void rwlocker::write_lock()
{
#ifdef _WIN64
    AcquireSRWLockExclusive(&rwlocker_->srwlock);
    rwlocker_->rw_mode = true;
#else
    pthread_rwlock_wrlock(&rwlocker_->rwlock);
#endif
}

void rwlocker::unlock()
{
#ifdef _WIN64
    if (rwlocker_->rw_mode)
    {
        ReleaseSRWLockExclusive(&rwlocker_->srwlock);
    }
    else
    {
        ReleaseSRWLockShared(&rwlocker_->srwlock);
    }
#else
    pthread_rwlock_unlock(&rwlocker_->rwlock);
#endif
}

zcc_namespace_end;
