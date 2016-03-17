#include "zc.h"

typedef struct ZQUEUE_NODE ZQUEUE_NODE;
struct ZQUEUE_NODE {
	void *res;
	ZLINK_NODE node;
};

ZQUEUE *zqueue_create(void)
{
	ZQUEUE *zq;

	zq = (ZQUEUE *) zmalloc(sizeof(ZQUEUE));
	zq->current_res_count = 0;
	pthread_mutex_init(&(zq->locker), 0);
	pthread_cond_init(&(zq->cond), 0);
	zlink_init(&(zq->node_list));
	zlink_init(&(zq->res_list));

	return zq;
}

void zqueue_enter_resource(ZQUEUE * zq, void *res)
{
	ZQUEUE_NODE *qn;

	pthread_mutex_lock(&(zq->locker));
	zq->current_res_count++;
	qn = (ZQUEUE_NODE *) zmalloc(sizeof(ZQUEUE_NODE));
	qn->res = res;
	zlink_push(&(zq->res_list), &(qn->node));
	pthread_mutex_unlock(&(zq->locker));
}

void *zqueue_require(ZQUEUE * zq)
{
	ZLINK_NODE *ln;
	ZQUEUE_NODE *qn;
	void *res;

	pthread_mutex_lock(&(zq->locker));
	while (!(zq->current_res_count)) {
		pthread_cond_wait(&(zq->cond), &(zq->locker));
	}
	zq->current_res_count--;
	ln = zlink_shift(&(zq->res_list));
	qn = ZCONTAINER_OF(ln, ZQUEUE_NODE, node);
	res = qn->res;
	zlink_push(&(zq->node_list), ln);
	pthread_mutex_unlock(&(zq->locker));

	return res;
}

void zqueue_release(ZQUEUE * zq, void *res)
{
	ZLINK_NODE *ln;
	ZQUEUE_NODE *qn;

	pthread_mutex_lock(&(zq->locker));
	zq->current_res_count++;
	ln = zlink_shift(&(zq->node_list));
	qn = ZCONTAINER_OF(ln, ZQUEUE_NODE, node);
	qn->res = res;
	zlink_push(&(zq->res_list), ln);
	pthread_mutex_unlock(&(zq->locker));
	pthread_cond_signal(&(zq->cond));
}

void zqueue_free(ZQUEUE * zq)
{
	ZLINK_NODE *ln;
	ZQUEUE_NODE *qn;

	while ((ln = zlink_shift(&(zq->node_list)))) {
		qn = ZCONTAINER_OF(ln, ZQUEUE_NODE, node);
		zfree(qn);
	}
	while ((ln = zlink_shift(&(zq->res_list)))) {
		qn = ZCONTAINER_OF(ln, ZQUEUE_NODE, node);
		zfree(qn);
	}

	zfree(zq);
}
