/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "libzc.h"

zchain_t *zchain_create(void)
{
    zchain_t *zc;

    zc = (zchain_t *) zmalloc(sizeof(zchain_t));
    ZMLINK_INIT(zc->head);
    ZMLINK_INIT(zc->tail);
    zc->len = 0;

    return (zc);
}

void zchain_free(zchain_t * zc, void (*free_fn) (void *, void *), void *ctx)
{
    zchain_node_t *n, *next;

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

void zchain_free_STR(zchain_t * zc)
{
    zchain_node_t *n, *next;

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

void zchain_walk(zchain_t * zc, void (*walk_fn) (zchain_node_t *, void *), void *ctx)
{
    zchain_node_t *n;

    n = zc->head;
    for (; n; n = n->next) {
        if (walk_fn) {
            (*walk_fn) (n, ctx);
        }
    }
}

int zchain_attach_before(zchain_t * zc, zchain_node_t * n, zchain_node_t * before)
{
    ZMLINK_ATTACH_BEFORE(zc->head, zc->tail, n, prev, next, before);
    zc->len++;

    return zc->len;
}

int zchain_detach(zchain_t * zc, zchain_node_t * n)
{
    ZMLINK_DETACH(zc->head, zc->tail, n, prev, next);
    zc->len--;

    return (zc->len);
}

zchain_node_t *zchain_add_before(zchain_t * zc, void *value, zchain_node_t * before)
{
    zchain_node_t *n;

    n = (zchain_node_t *) zmalloc(sizeof(zchain_node_t));
    n->value = value;
    ZMLINK_INIT(n->prev);
    ZMLINK_INIT(n->next);
    zchain_attach_before(zc, n, before);

    return n;
}

zchain_node_t *zchain_delete(zchain_t * zc, zchain_node_t * n, char **value)
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
