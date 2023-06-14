/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zc.h"

static int zdictlong_zrbtree_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2)
{
    zdictlong_node_t *dn1, *dn2;

    dn1 = ZCONTAINER_OF(n1, zdictlong_node_t, rbnode);
    dn2 = ZCONTAINER_OF(n2, zdictlong_node_t, rbnode);

    return strcmp(dn1->key, dn2->key);
}

zdictlong_t *zdictlong_create()
{
    zdictlong_t *dictlong;
    dictlong = (zdictlong_t *) zmalloc(sizeof(zdictlong_t));
    dictlong->len = 0;
    zrbtree_init(&(dictlong->rbtree), zdictlong_zrbtree_cmp);
    return dictlong;
}

void zdictlong_free(zdictlong_t * dictlong)
{
    if (!dictlong) {
        return;
    }
    zdictlong_node_t *n;
    while ((n = zdictlong_first(dictlong))) {
        zrbtree_detach(&(dictlong->rbtree), &(n->rbnode));
        zfree(n->key);
        zfree(n);
    }
    zfree(dictlong);
}

void zdictlong_reset(zdictlong_t *dictlong)
{
    if (!dictlong) {
        return;
    }
    zdictlong_node_t *n;
    while ((n = zdictlong_first(dictlong))) {
        zrbtree_detach(&(dictlong->rbtree), &(n->rbnode));
        zfree(n->key);
        zfree(n);
    }
    dictlong->len = 0;
}

zdictlong_node_t *zdictlong_update(zdictlong_t *dictlong, const char *key, long long value)
{
    zdictlong_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    mp_n.key = (char *)(void *)key;
    rb_np = zrbtree_attach(&(dictlong->rbtree), &(mp_n.rbnode));

    if (rb_np != &(mp_n.rbnode)) {
        mp_np = ZCONTAINER_OF(rb_np, zdictlong_node_t, rbnode);
        mp_np->value = value;
    } else {
        mp_np = (zdictlong_node_t *) zmalloc(sizeof(zdictlong_node_t));
        mp_np->key = zstrdup(key);
        mp_np->value = value;
        zrbtree_replace_node(&(dictlong->rbtree), &(mp_n.rbnode), &(mp_np->rbnode));
        dictlong->len++;
    }

    return mp_np;
}

zdictlong_node_t *zdictlong_find(const zdictlong_t * dictlong, const char *key, long long *value)
{
    zdictlong_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value) {
        *value = 0;
    }
    mp_n.key = (char *)(void *)key;
    rb_np = zrbtree_find(&(dictlong->rbtree), &(mp_n.rbnode));
    if (rb_np) {
        mp_np = ZCONTAINER_OF(rb_np, zdictlong_node_t, rbnode);
        if (value) {
            *value = mp_np->value;
        }
        return mp_np;
    }

    return 0;
}

zdictlong_node_t *zdictlong_find_near_prev(const zdictlong_t * dictlong, const char *key, long long *value)
{
    zdictlong_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value) {
        *value = 0;
    }

    mp_n.key = (char *)(void *)key;
    rb_np = zrbtree_near_prev(&(dictlong->rbtree), &(mp_n.rbnode));
    if (rb_np) {
        mp_np = ZCONTAINER_OF(rb_np, zdictlong_node_t, rbnode);
        if (value) {
            *value = mp_np->value;
        }
        return mp_np;
    }

    return 0;
}

zdictlong_node_t *zdictlong_find_near_next(const zdictlong_t * dictlong, const char *key, long long *value)
{
    zdictlong_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value) {
        *value = 0;
    }

    mp_n.key = (char *)(void *)key;
    rb_np = zrbtree_near_next(&(dictlong->rbtree), &(mp_n.rbnode));
    if (rb_np) {
        mp_np = ZCONTAINER_OF(rb_np, zdictlong_node_t, rbnode);
        if (value) {
            *value = mp_np->value;
        }
        return mp_np;
    }

    return 0;
}

void zdictlong_delete_node(zdictlong_t * dictlong, zdictlong_node_t * n)
{
    zrbtree_detach(&(dictlong->rbtree), &(n->rbnode));
    zfree(n->key);
    zfree(n);
    dictlong->len--;
}

void zdictlong_delete(zdictlong_t * dictlong, const char *key)
{
    zdictlong_node_t *n;

    n = zdictlong_find(dictlong, key, 0);
    if (!n) {
        return;
    }
    zrbtree_detach(&(dictlong->rbtree), &(n->rbnode));
    zfree(n->key);
    zfree(n);
    dictlong->len--;
}

zdictlong_node_t *zdictlong_first(const zdictlong_t * dictlong)
{
    zrbtree_node_t *rn = zrbtree_first(&(dictlong->rbtree));
    if (rn) {
        return ZCONTAINER_OF(rn, zdictlong_node_t, rbnode);
    }
    return 0;
}

zdictlong_node_t *zdictlong_last(const zdictlong_t * dictlong)
{
    zrbtree_node_t *rn = zrbtree_last(&(dictlong->rbtree));
    if (rn) {
        return ZCONTAINER_OF(rn, zdictlong_node_t, rbnode);
    }
    return 0;
}

zdictlong_node_t *zdictlong_prev(const zdictlong_node_t * node)
{
    zrbtree_node_t *rn = zrbtree_prev(&(node->rbnode));
    if (rn) {
        return ZCONTAINER_OF(rn, zdictlong_node_t, rbnode);
    }
    return 0;
}

zdictlong_node_t *zdictlong_next(const zdictlong_node_t * node)
{
    zrbtree_node_t *rn = zrbtree_next(&(node->rbnode));
    if (rn) {
        return ZCONTAINER_OF(rn, zdictlong_node_t, rbnode);
    }
    return 0;
}

void zdictlong_debug_show(const zdictlong_t * dictlong)
{
    ZDICTLONG_WALK_BEGIN(dictlong, k, v) {
        zdebug_show("%s = %lld", k, v);
    } ZDICTLONG_WALK_END;
}
