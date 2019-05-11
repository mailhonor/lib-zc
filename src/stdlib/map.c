/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-10-12
 * ================================
 */

#include "zc.h"

static int zmap_zrbtree_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2)
{
    zmap_node_t *dn1, *dn2;

    dn1 = ZCONTAINER_OF(n1, zmap_node_t, rbnode);
    dn2 = ZCONTAINER_OF(n2, zmap_node_t, rbnode);

    return strcmp(dn1->key, dn2->key);
}

zmap_t *zmap_create()
{
    zmap_t *map;
    map = (zmap_t *) zmalloc(sizeof(zmap_t));
    map->len = 0;
    zrbtree_init(&(map->rbtree), zmap_zrbtree_cmp);
    return map;
}

void zmap_free(zmap_t *map)
{
    zmap_node_t *n;

    while ((n = zmap_first(map))) {
        zrbtree_detach(&(map->rbtree), &(n->rbnode));
        zfree(n->key);
        zfree(n);
    }
    zfree(map);
}

zmap_node_t *zmap_update(zmap_t * map, const char *key, const void *value, void **old_value)
{
    zmap_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (old_value) {
        *old_value = 0;
    }

    mp_n.key = (char *)(void *)key;
    rb_np = zrbtree_attach(&(map->rbtree), &(mp_n.rbnode));

    if (rb_np != &(mp_n.rbnode)) {
        mp_np = ZCONTAINER_OF(rb_np, zmap_node_t, rbnode);
        if (old_value) {
            *old_value = mp_np->value;
        }
        mp_np->value = (void *)value;
    } else {
        mp_np = (zmap_node_t *) zmalloc(sizeof(zmap_node_t));
        mp_np->key = zstrdup(key);
        mp_np->value = (void *)value;
        zrbtree_replace_node(&(map->rbtree), &(mp_n.rbnode), &(mp_np->rbnode));
        map->len++;
    }

    return mp_np;
}

zmap_node_t *zmap_find(const zmap_t * map, const char *key, void **value)
{
    zmap_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value) {
        *value = 0;
    }
    mp_n.key = (char *)(void *)key;
    rb_np = zrbtree_find(&(map->rbtree), &(mp_n.rbnode));
    if (rb_np) {
        mp_np = ZCONTAINER_OF(rb_np, zmap_node_t, rbnode);
        if (value) {
            *value = (mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

zmap_node_t *zmap_find_near_prev(const zmap_t * map, const char *key, void **value)
{
    zmap_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value) {
        *value = 0;
    }

    mp_n.key = (char *)(void *)key;
    rb_np = zrbtree_near_prev(&(map->rbtree), &(mp_n.rbnode));
    if (rb_np) {
        mp_np = ZCONTAINER_OF(rb_np, zmap_node_t, rbnode);
        if (value) {
            *value = (mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

zmap_node_t *zmap_find_near_next(const zmap_t * map, const char *key, void **value)
{
    zmap_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value) {
        *value = 0;
    }

    mp_n.key = (char *)(void *)key;
    rb_np = zrbtree_near_next(&(map->rbtree), &(mp_n.rbnode));
    if (rb_np) {
        mp_np = ZCONTAINER_OF(rb_np, zmap_node_t, rbnode);
        if (value) {
            *value = (char *)(mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

void zmap_delete_node(zmap_t * map, zmap_node_t * n, void **old_value)
{
    zrbtree_detach(&(map->rbtree), &(n->rbnode));
    zfree(n->key);
    if (old_value) {
        *old_value = n->value;
    }
    zfree(n);
    map->len--;
}

zbool_t zmap_delete(zmap_t * map, const char *key, void **old_value)
{
    zmap_node_t *n;

    n = zmap_find(map, key, 0);
    if (!n) {
        return 0;
    }
    zrbtree_detach(&(map->rbtree), &(n->rbnode));
    zfree(n->key);
    if (old_value) {
        *old_value = n->value;
    }
    zfree(n);
    map->len--;
    return 1;
}

void zmap_node_update(zmap_node_t *n, const void *value, void **old_value)
{
    if (old_value) {
        *old_value = n->value;
    }
    n->value = (void *)value;
}

zmap_node_t *zmap_first(const zmap_t * map)
{
    zrbtree_node_t *rn = zrbtree_first(&(map->rbtree));
    if (rn) {
        return ZCONTAINER_OF(rn, zmap_node_t, rbnode);
    }
    return 0;
}

zmap_node_t *zmap_last(const zmap_t * map)
{
    zrbtree_node_t *rn = zrbtree_last(&(map->rbtree));
    if (rn) {
        return ZCONTAINER_OF(rn, zmap_node_t, rbnode);
    }
    return 0;
}

zmap_node_t *zmap_prev(const zmap_node_t * node)
{
    zrbtree_node_t *rn = zrbtree_prev(&(node->rbnode));
    if (rn) {
        return ZCONTAINER_OF(rn, zmap_node_t, rbnode);
    }
    return 0;
}

zmap_node_t *zmap_next(const zmap_node_t * node)
{
    zrbtree_node_t *rn = zrbtree_next(&(node->rbnode));
    if (rn) {
        return ZCONTAINER_OF(rn, zmap_node_t, rbnode);
    }
    return 0;
}

void zmap_reset(zmap_t *map)
{
    for (zmap_node_t *n = zmap_first(map); n; n=zmap_first(map)) {
        zmap_delete_node(map, n, 0);
    }
}
