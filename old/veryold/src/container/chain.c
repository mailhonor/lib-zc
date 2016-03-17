#include "zc.h"

ZCHAIN *zchain_create(void)
{
	ZCHAIN *zc;

	zc = (ZCHAIN *) zmalloc(sizeof(ZCHAIN));
	ZMLINK_INIT(zc->head);
	ZMLINK_INIT(zc->tail);
	zc->len = 0;

	return (zc);
}

void zchain_free(ZCHAIN * zc, ZFN_STRUCTURE_FREE_2 free_fn, void *ctx)
{
	ZCHAIN_NODE *n, *next;

	n = zc->head;
	for (; n; n = next) {
		next = n->next;
		if (free_fn) {
			(*free_fn) (n->value, ctx);
		}
		zfree(n);
	}

	zfree(zc);
}

void zchain_free_STR(ZCHAIN * zc)
{
	ZCHAIN_NODE *n, *next;

	n = zc->head;
	for (; n; n = next) {
		next = n->next;
		if (n->value) {
			zfree(n->value);
		}
		zfree(n);
	}

	zfree(zc);
}

void zchain_walk(ZCHAIN * zc, void (*walk_fn) (ZCHAIN_NODE *))
{
	ZCHAIN_NODE *n;

	n = zc->head;
	for (; n; n = n->next) {
		if (walk_fn) {
			(*walk_fn) (n);
		}
	}
}

int zchain_attach_before(ZCHAIN * zc, ZCHAIN_NODE * n, ZCHAIN_NODE * before)
{
	ZCHAIN_NODE *head, *tail;

	head = zc->head;
	tail = zc->tail;
	ZMLINK_ATTACH_BEFORE(head, tail, n, prev, next, before);
	zc->head = head;
	zc->tail = tail;
	zc->len++;

	return zc->len;
}

int zchain_detach(ZCHAIN * zc, ZCHAIN_NODE * n)
{
	ZCHAIN_NODE *head, *tail;

	head = zc->head;
	tail = zc->tail;
	ZMLINK_DETACH(head, tail, n, prev, next);
	zc->head = head;
	zc->tail = tail;
	zc->len--;

	return (zc->len);
}

ZCHAIN_NODE *zchain_enter_before(ZCHAIN * zc, char *value, ZCHAIN_NODE * before)
{
	ZCHAIN_NODE *n;

	n = (ZCHAIN_NODE *) zmalloc(sizeof(ZCHAIN_NODE));
	n->value = value;
	ZMLINK_INIT(n->prev);
	ZMLINK_INIT(n->next);
	zchain_attach_before(zc, n, before);

	return n;
}

ZCHAIN_NODE *zchain_delete(ZCHAIN * zc, ZCHAIN_NODE * n, char **value)
{
	if (n == 0) {
		return 0;
	}
	if (value) {
		*value = n->value;
	}
	zchain_detach(zc, n);
	zfree(n);

	return n;
}
