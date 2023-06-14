/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */

/* 一遍一遍的实现类似功能, 够麻烦的, 这一点完全比不上 C++ 的模版 */
#include "zc.h"

static int zlongmap_zrbtree_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2)
{
    zlongmap_node_t *dn1, *dn2;

    dn1 = ZCONTAINER_OF(n1, zlongmap_node_t, rbnode);
    dn2 = ZCONTAINER_OF(n2, zlongmap_node_t, rbnode);

    int r = 0;
    if (dn1->key > dn2->key) {
        r = 1;
    } else if (dn1->key < dn2->key) {
        r = -1;
    }
    return r;
}

zlongmap_t *zlongmap_create()
{
    zlongmap_t *longmap;
    longmap = (zlongmap_t *) zmalloc(sizeof(zlongmap_t));
    longmap->len = 0;
    zrbtree_init(&(longmap->rbtree), zlongmap_zrbtree_cmp);
    return longmap;
}

void zlongmap_free(zlongmap_t *longmap)
{
    zlongmap_node_t *n;

    while ((n = zlongmap_first(longmap))) {
        zrbtree_detach(&(longmap->rbtree), &(n->rbnode));
        zfree(n);
    }
    zfree(longmap);
}

zlongmap_node_t *zlongmap_update(zlongmap_t * longmap, long long key, const void *value, void **old_value)
{
    zlongmap_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (old_value) {
        *old_value = 0;
    }

    mp_n.key = key;
    rb_np = zrbtree_attach(&(longmap->rbtree), &(mp_n.rbnode));

    if (rb_np != &(mp_n.rbnode)) {
        mp_np = ZCONTAINER_OF(rb_np, zlongmap_node_t, rbnode);
        if (old_value) {
            *old_value = mp_np->value;
        }
        mp_np->value = (void *)value;
    } else {
        mp_np = (zlongmap_node_t *) zmalloc(sizeof(zlongmap_node_t));
        mp_np->key = key;
        mp_np->value = (void *)value;
        zrbtree_replace_node(&(longmap->rbtree), &(mp_n.rbnode), &(mp_np->rbnode));
        longmap->len++;
    }

    return mp_np;
}

zlongmap_node_t *zlongmap_find(const zlongmap_t * longmap, long long key, void **value)
{
    zlongmap_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value) {
        *value = 0;
    }
    mp_n.key = key;
    rb_np = zrbtree_find(&(longmap->rbtree), &(mp_n.rbnode));
    if (rb_np) {
        mp_np = ZCONTAINER_OF(rb_np, zlongmap_node_t, rbnode);
        if (value) {
            *value = (mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

zlongmap_node_t *zlongmap_find_near_prev(const zlongmap_t * longmap, long long key, void **value)
{
    zlongmap_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value) {
        *value = 0;
    }

    mp_n.key = key;
    rb_np = zrbtree_near_prev(&(longmap->rbtree), &(mp_n.rbnode));
    if (rb_np) {
        mp_np = ZCONTAINER_OF(rb_np, zlongmap_node_t, rbnode);
        if (value) {
            *value = (mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

zlongmap_node_t *zlongmap_find_near_next(const zlongmap_t * longmap, long long key, void **value)
{
    zlongmap_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value) {
        *value = 0;
    }

    mp_n.key = key;
    rb_np = zrbtree_near_next(&(longmap->rbtree), &(mp_n.rbnode));
    if (rb_np) {
        mp_np = ZCONTAINER_OF(rb_np, zlongmap_node_t, rbnode);
        if (value) {
            *value = (char *)(mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

void zlongmap_delete_node(zlongmap_t * longmap, zlongmap_node_t * n, void **old_value)
{
    zrbtree_detach(&(longmap->rbtree), &(n->rbnode));
    if (old_value) {
        *old_value = n->value;
    }
    zfree(n);
    longmap->len--;
}

zbool_t zlongmap_delete(zlongmap_t * longmap, long long key, void **old_value)
{
    zlongmap_node_t *n;

    n = zlongmap_find(longmap, key, 0);
    if (!n) {
        return 0;
    }
    zrbtree_detach(&(longmap->rbtree), &(n->rbnode));
    if (old_value) {
        *old_value = n->value;
    }
    zfree(n);
    longmap->len--;
    return 1;
}

void zlongmap_node_update(zlongmap_node_t *n, const void *value, void **old_value)
{
    if (old_value) {
        *old_value = n->value;
    }
    n->value = (void *)value;
}

zlongmap_node_t *zlongmap_first(const zlongmap_t * longmap)
{
    zrbtree_node_t *rn = zrbtree_first(&(longmap->rbtree));
    if (rn) {
        return ZCONTAINER_OF(rn, zlongmap_node_t, rbnode);
    }
    return 0;
}

zlongmap_node_t *zlongmap_last(const zlongmap_t * longmap)
{
    zrbtree_node_t *rn = zrbtree_last(&(longmap->rbtree));
    if (rn) {
        return ZCONTAINER_OF(rn, zlongmap_node_t, rbnode);
    }
    return 0;
}

zlongmap_node_t *zlongmap_prev(const zlongmap_node_t * node)
{
    zrbtree_node_t *rn = zrbtree_prev(&(node->rbnode));
    if (rn) {
        return ZCONTAINER_OF(rn, zlongmap_node_t, rbnode);
    }
    return 0;
}

zlongmap_node_t *zlongmap_next(const zlongmap_node_t * node)
{
    zrbtree_node_t *rn = zrbtree_next(&(node->rbnode));
    if (rn) {
        return ZCONTAINER_OF(rn, zlongmap_node_t, rbnode);
    }
    return 0;
}

void zlongmap_reset(zlongmap_t *longmap)
{
    for (zlongmap_node_t *n = zlongmap_first(longmap); n; n=zlongmap_first(longmap)) {
        zlongmap_delete_node(longmap, n, 0);
    }
}
