/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zc.h"

void zlist_init(zlist_t *list)
{
    memset(list, 0, sizeof(zlist_t));
}

void zlist_fini(zlist_t *list)
{
    zlist_node_t *n, *next;
    if (!list) {
        return;
    }
    n = list->head;
    for (; n; n = next) {
        next = n->next;
        zfree(n);
    }
}

zlist_t *zlist_create(void)
{
    zlist_t *list = zmalloc(sizeof(zlist_t));
    zlist_init(list);
    return (list);
}

void zlist_free(zlist_t * list)
{
    zlist_fini(list);
    zfree(list);
}

void zlist_reset(zlist_t *list)
{
    while(list->len) {
        zlist_pop(list, 0);
    }
}

void zlist_attach_before(zlist_t * list, zlist_node_t * n, zlist_node_t * before)
{
    ZMLINK_ATTACH_BEFORE(list->head, list->tail, n, prev, next, before);
    list->len++;
}

void zlist_detach(zlist_t * list, zlist_node_t * n)
{
    ZMLINK_DETACH(list->head, list->tail, n, prev, next);
    list->len--;
}

zlist_node_t *zlist_add_before(zlist_t * list, const void *value, zlist_node_t * before)
{
    zlist_node_t *n;

    n = (zlist_node_t *) zcalloc(1, sizeof(zlist_node_t));
    n->value = (char *)value;
    zlist_attach_before(list, n, before);

    return n;
}

int zlist_delete(zlist_t * list, zlist_node_t * n, void **value)
{
    if (n == 0) {
        return 0;
    }
    if (value) {
        *value = n->value;
    }
    zlist_detach(list, n);
    zfree(n);

    return 1;
}
