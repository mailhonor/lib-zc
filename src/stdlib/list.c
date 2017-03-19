/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "zc.h"

zlist_t *zlist_create(void)
{
    zlist_t *zc;

    zc = (zlist_t *) zmalloc(sizeof(zlist_t));
    ZMLINK_INIT(zc->head);
    ZMLINK_INIT(zc->tail);
    zc->len = 0;

    return (zc);
}

void zlist_free(zlist_t * zc)
{
    zlist_node_t *n, *next;

    n = zc->head;
    for (; n; n = next) {
        next = n->next;
        zfree(n);
    }

    zfree(zc);
}

void zlist_attach_before(zlist_t * zc, zlist_node_t * n, zlist_node_t * before)
{
    ZMLINK_ATTACH_BEFORE(zc->head, zc->tail, n, prev, next, before);
    zc->len++;
}

void zlist_detach(zlist_t * zc, zlist_node_t * n)
{
    ZMLINK_DETACH(zc->head, zc->tail, n, prev, next);
    zc->len--;
}

zlist_node_t *zlist_add_before(zlist_t * zc, const void *value, zlist_node_t * before)
{
    zlist_node_t *n;

    n = (zlist_node_t *) zmalloc(sizeof(zlist_node_t));
    n->value = (char *)value;
    ZMLINK_INIT(n->prev);
    ZMLINK_INIT(n->next);
    zlist_attach_before(zc, n, before);

    return n;
}

zbool_t zlist_delete(zlist_t * zc, zlist_node_t * n, char **value)
{
    if (n == 0) {
        return 0;
    }
    if (value) {
        *value = n->value;
    }
    zlist_detach(zc, n);
    zfree(n);

    return 1;
}
