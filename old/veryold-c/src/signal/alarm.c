#include "zc.h"

int zvar_alarm_signal = 32 + 5;

static int zalarm_used_pthread = 0;
static ZRBTREE zalarm_timer_tree;
static pthread_mutex_t zalarm_timer_locker = PTHREAD_MUTEX_INITIALIZER;

static timer_t zalarm_timer_inner;
static struct sigevent zalarm_timer_sev;

static int zalarm_time_cmp(ZRBTREE_NODE * n1, ZRBTREE_NODE * n2);
static void zalarm_signal_action(int sig, siginfo_t * si, void *uc);

void zalarm_base_init(int sig, int used_pthread)
{
	struct sigaction sa;

	zalarm_used_pthread = used_pthread;

	zrbtree_init(&zalarm_timer_tree, zalarm_time_cmp);

	if (sig > 0) {
		zvar_alarm_signal = sig;
	}
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = zalarm_signal_action;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, zvar_alarm_signal);
	if (sigaction(zvar_alarm_signal, &sa, NULL) == -1) {
		zlog_fatal("zalarm sigaction: %m");
	}

	zalarm_timer_sev.sigev_notify = SIGEV_SIGNAL;
	zalarm_timer_sev.sigev_signo = zvar_alarm_signal;
	zalarm_timer_sev.sigev_value.sival_ptr = &zalarm_timer_inner;
	timer_create(CLOCK_REALTIME, &zalarm_timer_sev, &zalarm_timer_inner);
}

void zalarm_base_fini(void)
{
	ZALARM *ala;
	ZRBTREE_NODE *rn;
	struct itimerspec its;

	if (zalarm_used_pthread) {
		pthread_mutex_lock(&zalarm_timer_locker);
	}

	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;
	timer_settime(&zalarm_timer_inner, 0, &its, 0);

	while (1) {
		rn = zrbtree_first(&zalarm_timer_tree);
		if (!rn) {
			break;
		}
		ala = ZCONTAINER_OF(rn, ZALARM, rbnode_time);
		zrbtree_detach(&zalarm_timer_tree, &(ala->rbnode_time));
	}

	if (zalarm_used_pthread) {
		pthread_mutex_unlock(&zalarm_timer_locker);
	}
}

static int zalarm_time_cmp(ZRBTREE_NODE * n1, ZRBTREE_NODE * n2)
{
	ZALARM *t1, *t2;
	int r;

	t1 = ZCONTAINER_OF(n1, ZALARM, rbnode_time);
	t2 = ZCONTAINER_OF(n2, ZALARM, rbnode_time);

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

static void zalarm_signal_action(int sig, siginfo_t * si, void *uc)
{
	ZALARM *ala;
	ZRBTREE_NODE *rn;
	struct itimerspec its;
	long delay;
	ZALARM_CB_FN callback;
	void *context;

	while (1) {
		delay = 0;
		rn = 0;
		if (zalarm_used_pthread) {
			pthread_mutex_lock(&zalarm_timer_locker);
		}
		rn = zrbtree_first(&zalarm_timer_tree);
		if (!rn) {
			break;
		}
		ala = ZCONTAINER_OF(rn, ZALARM, rbnode_time);
		delay = zmtime_left(ala->timeout);
		if (delay > 0) {
			break;
		}
		callback = ala->callback;
		context = ala->context;
		zrbtree_detach(&zalarm_timer_tree, &(ala->rbnode_time));
		ala->in_time = 0;
		if (zalarm_used_pthread) {
			pthread_mutex_unlock(&zalarm_timer_locker);
		}
		if (callback) {
			callback(ala, context);
		}
	}
	if (zalarm_used_pthread) {
		pthread_mutex_unlock(&zalarm_timer_locker);
	}

	if (rn) {
		ala = ZCONTAINER_OF(rn, ZALARM, rbnode_time);
		its.it_value.tv_sec = (long)(ala->timeout / 1000);
		its.it_value.tv_nsec = (long)((ala->timeout % 1000) * 1000);
		its.it_interval.tv_sec = 0;
		its.it_interval.tv_nsec = 0;
		timer_settime(&zalarm_timer_inner, TIMER_ABSTIME, &its, 0);
	}
}

void zalarm_init(ZALARM * ala)
{
}

void zalarm_fini(ZALARM * ala)
{
	zalarm_set(ala, 0, 0, 0);
}

int zalarm_set(ZALARM * ala, ZALARM_CB_FN callback, void *context, int timeout)
{
	ZRBTREE_NODE *rn;
	struct itimerspec its;

	if (zalarm_used_pthread) {
		pthread_mutex_lock(&zalarm_timer_locker);
	}

	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;
	timer_settime(&zalarm_timer_inner, 0, &its, 0);

	if ((ala->in_time) && (timeout != -1)) {
		zrbtree_detach(&zalarm_timer_tree, &(ala->rbnode_time));
		ala->in_time = 0;
	}
	if (timeout > 0) {
		ala->callback = callback;
		ala->context = context;
		ala->in_time = 1;
		ala->enable_time = 1;
		ala->timeout = zmtime_set_timeout(timeout);
		zrbtree_attach(&zalarm_timer_tree, &(ala->rbnode_time));
		ala->in_time = 1;
	} else if (timeout == 0) {
		ala->enable_time = 0;
	} else if (timeout == -1) {
		if ((ala->enable_time) && (!ala->in_time)) {
			zrbtree_attach(&zalarm_timer_tree, &(ala->rbnode_time));
		}
	}
	rn = zrbtree_first(&zalarm_timer_tree);
	if (rn) {
		ala = ZCONTAINER_OF(rn, ZALARM, rbnode_time);
		its.it_value.tv_sec = ala->timeout / 1000;
		its.it_value.tv_nsec = (ala->timeout % 1000) * 1000;
		its.it_interval.tv_sec = 0;
		its.it_interval.tv_nsec = 0;
		timer_settime(&zalarm_timer_inner, TIMER_ABSTIME, &its, 0);
	}

	if (zalarm_used_pthread) {
		pthread_mutex_unlock(&zalarm_timer_locker);
	}

	return 0;
}
