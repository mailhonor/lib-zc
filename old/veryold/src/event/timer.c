#include "zc.h"

static int ztimer_cmp(ZRBTREE_NODE * n1, ZRBTREE_NODE * n2)
{
	ZTIMER *t1, *t2;
	int r;

	t1 = ZCONTAINER_OF(n1, ZTIMER, rbnode_time);
	t2 = ZCONTAINER_OF(n2, ZTIMER, rbnode_time);

	r = t1->timeout - t2->timeout;

	/* FIXME */
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

void ztimer_base_init(ZRBTREE * timer_tree)
{
	zrbtree_init(timer_tree, ztimer_cmp);
}

int ztimer_check(ZRBTREE * timer_tree)
{
	ZTIMER *zt;
	ZRBTREE_NODE *rn;
	long delay = 0;
	ZTIMER_CB_FN callback;
	void *context;

	if (!zrbtree_have_data(timer_tree)) {
		return delay;
	}
	while (1) {
		delay = 0;
		rn = zrbtree_first(timer_tree);
		if (!rn) {
			break;
		}
		zt = ZCONTAINER_OF(rn, ZTIMER, rbnode_time);
		delay = zmtime_left(zt->timeout);
		if (delay > 0) {
			break;
		}
		callback = zt->callback;
		context = zt->context;
		zrbtree_detach(timer_tree, &(zt->rbnode_time));
		zt->in_time = 0;
		if (callback) {
			callback(zt, context);
		}
	}

	return delay;
}

void ztimer_init(ZTIMER * zt, ZEVENT_BASE * eb)
{
	memset(zt, 0, sizeof(ZTIMER));
	zt->event_base = eb;
}

void ztimer_fini(ZTIMER * zt)
{
	ztimer_set(zt, 0, 0, 0);
}

int ztimer_set(ZTIMER * zt, ZTIMER_CB_FN callback, void *context, int timeout)
{
	ZRBTREE *timer_tree;

	timer_tree = &(zt->event_base->timer_tree);

	if ((zt->in_time) && (timeout != -1)) {
		zrbtree_detach(timer_tree, &(zt->rbnode_time));
		zt->in_time = 0;
	}
	if (timeout > 0) {
		zt->callback = callback;
		zt->context = context;
		zt->in_time = 1;
		zt->enable_time = 1;
		zt->timeout = zmtime_set_timeout(timeout);
		zrbtree_attach(timer_tree, &(zt->rbnode_time));
		zt->in_time = 1;
	} else if (timeout == 0) {
		zt->enable_time = 0;
	} else if (timeout == -1) {
		if ((zt->enable_time) && (!zt->in_time)) {
			zrbtree_attach(timer_tree, &(zt->rbnode_time));
		}
	}

	return 0;
}
