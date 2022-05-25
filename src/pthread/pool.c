/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2018-12-02
 * ================================
 */


#include "zc.h"
#include <pthread.h>
#include <signal.h>

typedef struct _data_node_t _data_node_t;
struct _data_node_t {
    void (*fn)(void *ctx);
    void *ctx;
};

typedef struct _timeout_node_t _timeout_node_t;
struct _timeout_node_t {
    zrbtree_node_t rbnode_time;
    long cutoff_time;
    void (*fn)(void *ctx);
    void *ctx;
};

struct zpthread_pool_t {
    void (*pthread_init_handler)(zpthread_pool_t *ppool);
    void (*pthread_fini_handler)(zpthread_pool_t *ppool);
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    zmqueue_t *data_node_queue;
    zrbtree_t timeout_tree;
    void *ctx;
    int queue_length;
    int timeout_length;
    int min_count;
    int max_count;
    int current_count;
    int idle_count;
    int idle_timeout;
    int soft_stop_flag:2;
    int start_flag:2;
    int first_worker_flag:2;
    int timeout_flag:2;
    int debug_flag:2;
    zlink_t pthread_node_link;
};

typedef struct _pthread_node_t _pthread_node_t;
struct _pthread_node_t {
    zpthread_pool_t *ptp;
    zlink_node_t link_node;
    long job_start_time;
    int first_worker_flag:2;
    int timeout_flag:2;
    int enter_flag_for_idle:2;
};

#define POOL_DEBUG_STATUS(title) if (ptp->debug_flag) { \
    _pool_show_status_debug(ptp, title, __FILE__, __LINE__); \
}
#define mydebug                  if (ptp->debug_flag) zinfo

#define POOL_INNER_LOCK() { zpthread_lock(&(ptp->mutex)); }
#define POOL_INNER_UNLOCK() { zpthread_unlock(&(ptp->mutex)); }

static __thread zpthread_pool_t *_current_ptp = 0;

static int _pool_timeout_tree_cmp(zrbtree_node_t *n1, zrbtree_node_t *n2);

static void _pool_show_status_debug(zpthread_pool_t *ptp, const char *title, const char *fn, size_t ln)
{
    zlog_info(fn, ln, "### PTHREAD_POOL: %s, tid:%ld current:%d, idle:%d, queue:%d, timer:%d", title, zgettid(), ptp->current_count, ptp->idle_count, ptp->queue_length, ptp->timeout_length);
}

zpthread_pool_t *zpthread_pool_create()
{
    zpthread_pool_t *ptp  = (zpthread_pool_t *)zcalloc(1, sizeof(zpthread_pool_t));
    pthread_mutex_init(&(ptp->mutex), 0);
    pthread_cond_init(&(ptp->cond), 0);
    ptp->data_node_queue = zmqueue_create(sizeof(_data_node_t), 128);
    zlink_init(&(ptp->pthread_node_link));
    zrbtree_init(&(ptp->timeout_tree), _pool_timeout_tree_cmp);
    ptp->min_count = 0;
    ptp->max_count = 1;
    ptp->idle_timeout = 60;
    return ptp;
}

void zpthread_pool_free(zpthread_pool_t *ptp)
{
    if (!ptp) {
        return;
    }
    zmqueue_free(ptp->data_node_queue);
    zlink_fini(&(ptp->pthread_node_link));
    if (zvar_memleak_check) {
        zrbtree_node_t *rn;
        while((rn = zrbtree_first(&(ptp->timeout_tree)))) {
            zrbtree_detach(&(ptp->timeout_tree), rn);
            _timeout_node_t *tn = ZCONTAINER_OF(rn, _timeout_node_t, rbnode_time);
            zfree(tn);
        }
    }
    zfree(ptp);
}

long zpthread_pool_get_max_running_millisecond(zpthread_pool_t *ptp)
{
    if (!ptp) {
        return 0;
    }
    long min_time = 0;
    POOL_INNER_LOCK();
    for (zlink_node_t *ln = zlink_head(&(ptp->pthread_node_link)); ln; ln = zlink_node_next(ln)) {
        _pthread_node_t *pn = ZCONTAINER_OF(ln, _pthread_node_t, link_node);
        if (!pn) {
            continue;
        }
        if ((pn->job_start_time > 0)) {
            if (min_time == 0) {
                min_time = pn->job_start_time;
            }
            if (pn->job_start_time < min_time) {
                min_time = pn->job_start_time;
            }
        }
    }
    POOL_INNER_UNLOCK();
    if (min_time == 0) {
        return 0;
    }
    return (zmillisecond()- min_time);
}

void zpthread_pool_set_debug_flag(zpthread_pool_t *ptp, zbool_t flag)
{
    ptp->debug_flag = flag;
}

void zpthread_pool_set_min_max_count(zpthread_pool_t *ptp, int min, int max)
{
    if (min < 1) {
        min = 1;
    }
    if (min > 1024) {
        zinfo("WARNING zpthread_pool_set_max_min_count min(%d>1024)", min);
    }
    ptp->min_count = min;

    if (max < 1) {
        max = 1;
    }
    if (max > 10240) {
        zinfo("WARNING zpthread_pool_set_max_min_count max(%d>10240)", max);
    }
    ptp->max_count = max;
}

void zpthread_pool_set_idle_timeout(zpthread_pool_t *ptp, int timeout)
{
    ptp->idle_timeout = (timeout < 1?1:timeout);
}

int zpthread_pool_get_current_count(zpthread_pool_t *ptp)
{
    return ptp->current_count;
}

int zpthread_pool_get_queue_length(zpthread_pool_t *ptp)
{
    return ptp->queue_length;
}

void zpthread_pool_set_pthread_init_handler(zpthread_pool_t *ptp, void (*pthread_init_handler)(zpthread_pool_t *ptp))
{
    ptp->pthread_init_handler = pthread_init_handler;
}

void zpthread_pool_set_pthread_fini_handler(zpthread_pool_t *ptp, void (*pthread_fini_handler)(zpthread_pool_t *ptp))
{
    ptp->pthread_fini_handler = pthread_fini_handler;
}

void zpthread_pool_set_context(zpthread_pool_t *ptp, void *ctx)
{
    ptp->ctx = ctx;
}

void *zpthread_pool_get_context(zpthread_pool_t *ptp)
{
    return ptp->ctx;
}

zpthread_pool_t *zpthread_pool_get_current_zpthread_pool()
{
    return _current_ptp;
}

void zpthread_pool_softstop(zpthread_pool_t *ptp)
{
    if (!ptp) {
        return;
    }
    ptp->soft_stop_flag = 1;
    pthread_cond_broadcast(&(ptp->cond));
}

void zpthread_pool_wait_all_stopped(zpthread_pool_t *ptp, int max_second)
{
    if (!ptp) {
        return;
    }

    zpthread_pool_softstop(ptp);

    if (max_second > 1024 * 1024) {
        max_second = 1024 * 1024;
    }
    max_second *= 10;

    for (int i = 0; i < max_second; i++) {
        if (ptp->current_count < 1) {
            break;
        }
        zsleep_millisecond(100);
    }
}

static void *_pool_worker_run(void *arg);
static void _pool_start_one_pthread(zpthread_pool_t *ptp)
{
    pthread_t pth;
    if (pthread_create(&pth, 0, _pool_worker_run, ptp)) {
        zfatal("create pthread(%m)");
    }
}

void zpthread_pool_start(zpthread_pool_t *ptp)
{
    ptp->start_flag = 1;
    for (int i = 0; i < ptp->min_count; i++) {
        POOL_INNER_LOCK();
        ptp->idle_count++;
        ptp->current_count++;
        POOL_INNER_UNLOCK();
        _pool_start_one_pthread(ptp);
    }
}

static _pthread_node_t *_pool_worker_run_init(zpthread_pool_t *ptp)
{
    pthread_detach(pthread_self());

    _pthread_node_t *ptn = (_pthread_node_t *)zcalloc(1, sizeof(_pthread_node_t));
    ptn->ptp = ptp;

    POOL_INNER_LOCK();
    zlink_push(&(ptp->pthread_node_link), &(ptn->link_node));
    if (ptp->first_worker_flag == 0) {
        ptp->first_worker_flag = 1;
        ptn->first_worker_flag = 1;
    }
    POOL_INNER_UNLOCK();

    if (ptp->pthread_init_handler) {
        ptp->pthread_init_handler(ptp);
    }
    ptn->enter_flag_for_idle = 1;
    return ptn;
}

static void _pool_worker_run_fini(_pthread_node_t *ptn)
{
    zpthread_pool_t *ptp = ptn->ptp;
    if (ptp->pthread_fini_handler) {
        ptp->pthread_fini_handler(ptn->ptp);
    }
    int timeout_flag = ptn->timeout_flag;
    zfree(ptn);
    POOL_INNER_LOCK();
    ptp->idle_count--;
    ptp->current_count--;
    if (timeout_flag) {
        ptp->timeout_flag = 0;
    }
    POOL_DEBUG_STATUS("_pool_worker_run_fini");
    POOL_INNER_UNLOCK();
}

static _timeout_node_t *_pool_worker_run_get_timeout_node(_pthread_node_t *ptn, struct timespec *timedwait)
{
    zpthread_pool_t *ptp = ptn->ptp;
    zrbtree_node_t *rn;
    _timeout_node_t *tn;
    long curtime, left, sec, nsec;

    curtime = zmillisecond();

    if (!(rn = zrbtree_first(&(ptp->timeout_tree)))) {
        timedwait->tv_sec = curtime/1000 + 1;
        timedwait->tv_nsec = 0;
        return 0;
    } 

    tn = ZCONTAINER_OF(rn, _timeout_node_t, rbnode_time);
    if (curtime >= tn->cutoff_time) {
        zrbtree_detach(&(ptp->timeout_tree), rn);
        return tn;
    }

    left = tn->cutoff_time - curtime;
    sec = left/1000;
    nsec = (left%1000) * 1000 * 1000;
    if (sec > 0) {
        timedwait->tv_sec = curtime/1000 + 1;
        timedwait->tv_nsec = 0;
    } else {
        if (nsec < 100 * 1000 * 1000) {
            nsec = 100 * 1000 * 1000;
        }
        timedwait->tv_sec = curtime/1000;
        timedwait->tv_nsec = nsec;
    }
    return 0;
}

static zbool_t _pool_worker_run_get(_pthread_node_t *ptn, _data_node_t *dnode)
{
    zpthread_pool_t *ptp = ptn->ptp;
    struct timespec timedwait;
    _data_node_t *dn = 0;
    _timeout_node_t *tn = 0;
    int left_timeout_flag = 0;
    int need_create = 0;
    long curstamp = time(0);

    POOL_INNER_LOCK();
    if (ptn->enter_flag_for_idle) {
        ptn->enter_flag_for_idle = 0;
    } else {
        ptp->idle_count++;
    }

    while ((!zvar_sigint_flag) && (!(ptp->soft_stop_flag))) {
        left_timeout_flag = 0;
        if (ptn->first_worker_flag && (zrbtree_have_data(&(ptp->timeout_tree)))) {
            tn = _pool_worker_run_get_timeout_node(ptn, &timedwait);
            left_timeout_flag = 1;
            if (tn) {
                break;
            }
        }
        if (ptp->queue_length) {
            break;
        }
        if (!left_timeout_flag) {
            timedwait.tv_sec = time(0) + 1;
            timedwait.tv_nsec = 0;
        }
        if (ptp->timeout_flag == 0) {
            if (ptn->first_worker_flag == 0) {
                if (ptp->min_count != ptp->max_count) {
                    if (ptp->current_count > ptp->min_count ) {
                        if (timedwait.tv_sec - curstamp > ptp->idle_timeout + 1) {
                            ptn->timeout_flag = 1;
                            POOL_DEBUG_STATUS("worker idle timeout");
                            break;
                        }
                    }
                }
            }
        }
        pthread_cond_timedwait(&(ptp->cond), &(ptp->mutex), &timedwait);
    }
    if (tn) {
        dnode->fn = tn->fn;
        dnode->ctx = tn->ctx;
        zfree(tn);
        ptp->idle_count --;
    } else if ((!zvar_sigint_flag) && (!(ptp->soft_stop_flag)) && (ptp->queue_length)) {
        dn = (_data_node_t *)zmqueue_get_head(ptp->data_node_queue);
        dnode->fn = dn->fn;
        dnode->ctx = dn->ctx;
        zmqueue_release_and_shift(ptp->data_node_queue);
        ptp->queue_length --;
        ptp->idle_count --;
        need_create = 0;
        if ((ptp->queue_length > 0) && (ptp->idle_count < 1)) {
            if (ptp->current_count < ptp->max_count) {
                need_create = 1;
                ptp->idle_count++;
                ptp->current_count++;
            }
        }
    }
    POOL_INNER_UNLOCK();

    POOL_DEBUG_STATUS("worker get return");
    mydebug("worker get return, job: %d, timer:%d", dn?1:0, tn?1:0);
    if (tn) {
        mydebug("strike timer");
    }
    if (dn) {
        mydebug("strike job");
    }
    if (need_create) {
        mydebug("worker inner, need create one");
        _pool_start_one_pthread(ptp);
    }
    return ((dn||tn)?1:0);
}

static void *_pool_worker_run(void *arg)
{
    zpthread_pool_t *ptp = (zpthread_pool_t *)arg;
    _pthread_node_t *ptn = _pool_worker_run_init(ptp);
    _data_node_t dnode = { 0, 0 };
    POOL_DEBUG_STATUS("worker start");
    while ((!zvar_sigint_flag) && (!(ptp->soft_stop_flag) && (!(ptn->timeout_flag)))) {
        if (!_pool_worker_run_get(ptn, &dnode)) {
            continue;
        }
        ptn->job_start_time = zmillisecond();
        dnode.fn(dnode.ctx);
        ptn->job_start_time = 0;
    }
    _pool_worker_run_fini(ptn);
    return arg;
}

void zpthread_pool_job(zpthread_pool_t *ptp, void (*callback)(void *ctx), void *ctx)
{
    if (!callback) {
        return;
    }
    if ((!ptp) || (ptp->soft_stop_flag)) {
        callback(ctx);
        return;
    }

    int need_create = 0;
    _data_node_t *dnode = 0;
    POOL_INNER_LOCK();
    dnode = zmqueue_require_and_push(ptp->data_node_queue);
    dnode->fn = callback;
    dnode->ctx = ctx;
    ptp->queue_length += 1;
    if (ptp->idle_count < 1) {
        if (ptp->current_count < ptp->max_count) {
            need_create = 1;
            ptp->idle_count++;
            ptp->current_count++;
        }
    }
    POOL_DEBUG_STATUS("new job");
    POOL_INNER_UNLOCK();
    pthread_cond_signal(&(ptp->cond));
    if (need_create) {
        mydebug("new job, need create one");
        _pool_start_one_pthread(ptp);
    }
}

static int _pool_timeout_tree_cmp(zrbtree_node_t *n1, zrbtree_node_t *n2)
{
    _timeout_node_t *t1 = ZCONTAINER_OF(n1, _timeout_node_t, rbnode_time);
    _timeout_node_t *t2 = ZCONTAINER_OF(n2, _timeout_node_t, rbnode_time);

    long r = t1->cutoff_time - t2->cutoff_time;
    if (!r) {
        r = (char *)t1 - (char *)t2;
    }
    if (r > 0) {
        return 1;
    } else if (r < 0) {
        return -1;
    }
    return 0;
}

void zpthread_pool_timer(zpthread_pool_t *ptp, void (*callback)(void *ctx), void *ctx, int timeout)
{
    if (!callback) {
        return;
    }
    if ((!ptp) || (ptp->soft_stop_flag)) {
        return;
    }

    _timeout_node_t *tnode = (_timeout_node_t *)zcalloc(1, sizeof(_timeout_node_t));
    POOL_INNER_LOCK();
    tnode->fn = callback;
    tnode->ctx = ctx;
    tnode->cutoff_time = ztimeout_set_millisecond(timeout * 1000);
    zrbtree_attach(&(ptp->timeout_tree), &(tnode->rbnode_time));
    POOL_DEBUG_STATUS("new timer");
    POOL_INNER_UNLOCK();
}

