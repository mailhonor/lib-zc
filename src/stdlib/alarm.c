/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-20
 * ================================
 */

#include "libzc.h"
#include <pthread.h>
#include <signal.h>

typedef struct zalart_env_t zalart_env_t;
struct zalart_env_t {
    int use_lock;
    int signal;
    pthread_mutex_t lock;
    timer_t timer;
    struct sigevent timer_sev;
    zrbtree_t tree;
};

static zalart_env_t *___env = 0;
static void ___env_create(void);
static int ___tree_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2);
static void ___signal_action(int sig, siginfo_t * si, void *uc);

static void ___env_create(void)
{
    if (___env) {
        return;
    }
    ___env = (zalart_env_t *) zcalloc(1, sizeof(zalart_env_t));
    ___env->use_lock = 0;
    ___env->signal = 32 + 5;
}

void zalarm_use_lock(void)
{
    ___env_create();
    ___env->use_lock = 1;
    pthread_mutex_init(&(___env->lock), 0);
}

void zalarm_set_sig(int sig)
{
    ___env_create();
    if (sig > 0) {
        ___env->signal = sig;
    }
}

void zalarm_env_init(void)
{
    struct sigaction sa;

    ___env_create();

    zrbtree_init(&(___env->tree), ___tree_cmp);

    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = ___signal_action;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, ___env->signal);
    if (sigaction(___env->signal, &sa, NULL) == -1) {
        zfatal("zalarm sigaction: %m");
    }

    ___env->timer_sev.sigev_notify = SIGEV_SIGNAL;
    ___env->timer_sev.sigev_signo = ___env->signal;
    ___env->timer_sev.sigev_value.sival_ptr = &(___env->timer);
    timer_create(CLOCK_REALTIME, &(___env->timer_sev), &(___env->timer));
}

void zalarm_env_fini(void)
{
    zalarm_t *alarm;
    zrbtree_node_t *rn;
    struct itimerspec its;

    if (___env->use_lock) {
        pthread_mutex_lock(&(___env->lock));
    }

    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    timer_settime(&(___env->timer), 0, &its, 0);

    while (1) {
        rn = zrbtree_first(&(___env->tree));
        if (!rn) {
            break;
        }
        alarm = ZCONTAINER_OF(rn, zalarm_t, rbnode_time);
        zrbtree_detach(&(___env->tree), &(alarm->rbnode_time));
    }

    if (___env->use_lock) {
        pthread_mutex_unlock(&(___env->lock));
    }

    zfree(___env);
}

static int ___tree_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2)
{
    zalarm_t *t1, *t2;
    int r;

    t1 = ZCONTAINER_OF(n1, zalarm_t, rbnode_time);
    t2 = ZCONTAINER_OF(n2, zalarm_t, rbnode_time);

    r = t1->timeout - t2->timeout;
    if (!r) {
        r = (char *)(n1) - (char *)(n2);
    }
    if (r > 0) {
        return 1;
    } else if (r < 0) {
        return -1;
    }

    return 0;
}

static void ___signal_action(int sig, siginfo_t * si, void *uc)
{
    zalarm_t *alarm;
    zrbtree_node_t *rn;
    struct itimerspec its;
    long delay;
    zalarm_cb_t callback;
    int auto_release;

    while (1) {
        delay = 0;
        rn = 0;
        if (___env->use_lock) {
            pthread_mutex_lock(&(___env->lock));
        }
        rn = zrbtree_first(&(___env->tree));
        if (!rn) {
            break;
        }
        alarm = ZCONTAINER_OF(rn, zalarm_t, rbnode_time);
        delay = ztimeout_left(alarm->timeout);
        if (delay > 0) {
            break;
        }
        callback = alarm->callback;
        zrbtree_detach(&(___env->tree), &(alarm->rbnode_time));
        alarm->in_time = 0;
        if (___env->use_lock) {
            pthread_mutex_unlock(&(___env->lock));
        }
        auto_release = alarm->auto_release;
        if (callback) {
            callback(alarm);
        }
        if (auto_release) {
            zalarm_free(alarm);
        }
    }
    if (___env->use_lock) {
        pthread_mutex_unlock(&(___env->lock));
    }

    if (rn) {
        alarm = ZCONTAINER_OF(rn, zalarm_t, rbnode_time);
        its.it_value.tv_sec = (long)(alarm->timeout / 1000);
        its.it_value.tv_nsec = (long)((alarm->timeout % 1000) * 1000);
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
        timer_settime(&(___env->timer), TIMER_ABSTIME, &its, 0);
    }
}

void zalarm_init(zalarm_t * alarm)
{
    memset(alarm, 0, sizeof(zalarm_t));
}

void zalarm_fini(zalarm_t * alarm)
{
    zalarm_set(alarm, 0, 0);
}

zalarm_t *zalarm_create(void)
{
    zalarm_t *alarm;

    alarm = (zalarm_t *) zcalloc(1, sizeof(zalarm_t));
    zalarm_init(alarm);

    return alarm;
}

void zalarm_free(zalarm_t * alarm)
{
    zalarm_fini(alarm);
    zfree(alarm);
}

void zalarm_set(zalarm_t * alarm, zalarm_cb_t callback, long timeout)
{
    zrbtree_node_t *rn;
    struct itimerspec its;

    if (___env->use_lock) {
        pthread_mutex_lock(&(___env->lock));
    }

    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    timer_settime(&(___env->timer), 0, &its, 0);

    if ((alarm->in_time) && (timeout != -1)) {
        zrbtree_detach(&(___env->tree), &(alarm->rbnode_time));
        alarm->in_time = 0;
    }
    if (timeout > 0) {
        alarm->callback = callback;
        alarm->in_time = 1;
        alarm->enable_time = 1;
        alarm->timeout = ztimeout_set(timeout);
        zrbtree_attach(&(___env->tree), &(alarm->rbnode_time));
    } else if (timeout == 0) {
        alarm->enable_time = 0;
    } else if (timeout == -1) {
        if ((alarm->enable_time) && (!alarm->in_time)) {
            zrbtree_attach(&(___env->tree), &(alarm->rbnode_time));
        }
    }
    rn = zrbtree_first(&(___env->tree));
    if (rn) {
        alarm = ZCONTAINER_OF(rn, zalarm_t, rbnode_time);
        its.it_value.tv_sec = alarm->timeout / 1000;
        its.it_value.tv_nsec = (alarm->timeout % 1000) * 1000;
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
        timer_settime(&(___env->timer), TIMER_ABSTIME, &its, 0);
    }

    if (___env->use_lock) {
        pthread_mutex_unlock(&(___env->lock));
    }
}

void zalarm_stop(zalarm_t * alarm)
{
    zalarm_set(alarm, 0, 0);
}

void zalarm_continue(zalarm_t * alarm)
{
    zalarm_set(alarm, 0, -1);
}

void zalarm(zalarm_cb_t callback, void *context, long timeout)
{
    zalarm_t *alarm;

    alarm = zalarm_create();
    alarm->auto_release = 1;
    zalarm_set_context(alarm, context);
    zalarm_set(alarm, callback, timeout);
}
