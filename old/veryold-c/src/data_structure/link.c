#include "zc.h"

void zlink_init(ZLINK * link)
{
	link->head = 0;
	link->tail = 0;
}

ZLINK_NODE *zlink_attach_before(ZLINK * link, ZLINK_NODE * node, ZLINK_NODE * before)
{
	ZMLINK_ATTACH_BEFORE(link->head, link->tail, node, prev, next, before);

	return node;
}

ZLINK_NODE *zlink_detach(ZLINK * link, ZLINK_NODE * node)
{
	if (!node) {
		return 0;
	}
	ZMLINK_DETACH(link->head, link->tail, node, prev, next);

	return node;
}

ZLINK_NODE *zlink_push(ZLINK * link, ZLINK_NODE * node)
{
	ZMLINK_APPEND(link->head, link->tail, node, prev, next);

	return node;
}

ZLINK_NODE *zlink_unshift(ZLINK * link, ZLINK_NODE * node)
{
	ZMLINK_PREPEND(link->head, link->tail, node, prev, next);

	return node;
}

ZLINK_NODE *zlink_pop(ZLINK * link)
{
	ZLINK_NODE *node;

	node = link->tail;
	if (node == 0) {
		return 0;
	}
	ZMLINK_DETACH(link->head, link->tail, node, prev, next);

	return node;
}

ZLINK_NODE *zlink_shift(ZLINK * link)
{
	ZLINK_NODE *node;

	node = link->head;
	if (node == 0) {
		return 0;
	}
	ZMLINK_DETACH(link->head, link->tail, node, prev, next);

	return node;
}

void zlink_fini(ZLINK * link, void (*fini_fn) (ZLINK_NODE *))
{
	ZLINK_NODE *n, *next;

	n = link->head;
	for (; n; n = next) {
		next = n->next;
		if (fini_fn) {
			(*fini_fn) (n);
		}
	}
}

void zlink_walk(ZLINK * link, void (*walk_fn) (ZLINK_NODE *))
{
	ZLINK_NODE *n, *next;

	n = link->head;
	for (; n; n = next) {
		next = n->next;
		if (walk_fn) {
			(*walk_fn) (n);
		}
	}
}
