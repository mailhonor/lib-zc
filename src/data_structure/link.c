/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-09-28
 * ================================
 */

#include "libzc.h"

void zlink_init(zlink_t * link)
{
    link->head = 0;
    link->tail = 0;
}

zlink_node_t *zlink_attach_before(zlink_t * link, zlink_node_t * node, zlink_node_t * before)
{
    ZMLINK_ATTACH_BEFORE(link->head, link->tail, node, prev, next, before);

    return node;
}

zlink_node_t *zlink_detach(zlink_t * link, zlink_node_t * node)
{
    if (!node) {
        return 0;
    }
    ZMLINK_DETACH(link->head, link->tail, node, prev, next);

    return node;
}

zlink_node_t *zlink_push(zlink_t * link, zlink_node_t * node)
{
    ZMLINK_APPEND(link->head, link->tail, node, prev, next);

    return node;
}

zlink_node_t *zlink_unshift(zlink_t * link, zlink_node_t * node)
{
    ZMLINK_PREPEND(link->head, link->tail, node, prev, next);

    return node;
}

zlink_node_t *zlink_pop(zlink_t * link)
{
    zlink_node_t *node;

    node = link->tail;
    if (node == 0) {
        return 0;
    }
    ZMLINK_DETACH(link->head, link->tail, node, prev, next);

    return node;
}

zlink_node_t *zlink_shift(zlink_t * link)
{
    zlink_node_t *node;

    node = link->head;
    if (node == 0) {
        return 0;
    }
    ZMLINK_DETACH(link->head, link->tail, node, prev, next);

    return node;
}

void zlink_fini(zlink_t * link, void (*fini_fn) (zlink_node_t *))
{
    zlink_node_t *n, *next;

    n = link->head;
    for (; n; n = next) {
        next = n->next;
        if (fini_fn) {
            (*fini_fn) (n);
        }
    }
}

void zlink_walk(zlink_t * link, void (*walk_fn) (zlink_node_t *))
{
    zlink_node_t *n, *next;

    n = link->head;
    for (; n; n = next) {
        next = n->next;
        if (walk_fn) {
            (*walk_fn) (n);
        }
    }
}
