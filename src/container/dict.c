/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "libzc.h"

static int zdict_rbtree_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2)
{
    zdict_node_t *dn1, *dn2;

    dn1 = ZCONTAINER_OF(n1, zdict_node_t, rbnode);
    dn2 = ZCONTAINER_OF(n2, zdict_node_t, rbnode);

    return strcmp(dn1->key, dn2->key);
}

zdict_t *zdict_create(void)
{
    return zdict_create_mpool(0);
}

zdict_t *zdict_create_mpool(zmpool_t *mpool)
{
    zdict_t *mp;

    mp = (zdict_t *) zmpool_malloc(mpool, sizeof(zdict_t));
    mp->mpool = mpool;
    mp->len = 0;
    zrbtree_init(&(mp->rbtree), zdict_rbtree_cmp);

    return mp;
}

void zdict_free(zdict_t * mp)
{
    zdict_node_t *n;

    while ((n = zdict_first(mp)))
    {
        zrbtree_detach(&(mp->rbtree), &(n->rbnode));
         zmpool_free(mp->mpool, n->value);
         zmpool_free(mp->mpool, n->key);
         zmpool_free(mp->mpool, n);
    }
    zmpool_free(mp->mpool, mp);
}

zdict_node_t *zdict_add(zdict_t * mp, char *key, char *value)
{
    zdict_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    mp_n.key = key;
    rb_np = zrbtree_attach(&(mp->rbtree), &(mp_n.rbnode));

    if (rb_np != &(mp_n.rbnode))
    {
        mp_np = ZCONTAINER_OF(rb_np, zdict_node_t, rbnode);
        zmpool_free(mp->mpool, mp_np->value);
        mp_np->value = zmpool_strdup(mp->mpool, value);
    }
    else
    {
        mp_np = (zdict_node_t *) zmpool_malloc(mp->mpool, sizeof(zdict_node_t));
        mp_np->key = zmpool_strdup(mp->mpool, key);
        mp_np->value = zmpool_strdup(mp->mpool, value);
        zrbtree_replace_node(&(mp->rbtree), &(mp_n.rbnode), &(mp_np->rbnode));
        mp->len ++;
    }

    return mp_np;
}

zdict_node_t *zdict_lookup(zdict_t * mp, char *key, char **value)
{
    zdict_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value)
    {
        *value = 0;
    }
    mp_n.key = key;
    rb_np = zrbtree_lookup(&(mp->rbtree), &(mp_n.rbnode));
    if (rb_np)
    {
        mp_np = ZCONTAINER_OF(rb_np, zdict_node_t, rbnode);
        if (value)
        {
            *value = (char *)(mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

zdict_node_t *zdict_lookup_near_prev(zdict_t * mp, char *key, char **value)
{
    zdict_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value)
    {
        *value = 0;
    }

    mp_n.key = key;
    rb_np = zrbtree_near_prev(&(mp->rbtree), &(mp_n.rbnode));
    if (rb_np)
    {
        mp_np = ZCONTAINER_OF(rb_np, zdict_node_t, rbnode);
        if (value)
        {
            *value = (char *)(mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

zdict_node_t *zdict_lookup_near_next(zdict_t * mp, char *key, char **value)
{
    zdict_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value)
    {
        *value = 0;
    }

    mp_n.key = key;
    rb_np = zrbtree_near_next(&(mp->rbtree), &(mp_n.rbnode));
    if (rb_np)
    {
        mp_np = ZCONTAINER_OF(rb_np, zdict_node_t, rbnode);
        if (value)
        {
            *value = (char *)(mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

void zdict_delete_node(zdict_t * mp, zdict_node_t *n)
{
    zrbtree_detach(&(mp->rbtree), &(n->rbnode));
         zmpool_free(mp->mpool, n->key);
    zmpool_free(mp->mpool, n->value);
    zmpool_free(mp->mpool, n);
    mp->len --;
}

void zdict_delete(zdict_t * mp, char *key)
{
    zdict_node_t *n;

    n = zdict_lookup(mp, key, 0);
    if (!n)
    {
        return;
    }
    zrbtree_detach(&(mp->rbtree), &(n->rbnode));
    zmpool_free(mp->mpool, n->key);
    zmpool_free(mp->mpool, n->value);
    zmpool_free(mp->mpool, n);
    mp->len --;
}

zdict_node_t *zdict_first(zdict_t * mp)
{
    zrbtree_node_t *rn = zrbtree_first(&(mp->rbtree));
    if (rn)
    {
        return ZCONTAINER_OF(rn, zdict_node_t, rbnode);
    }
    return 0;
}

zdict_node_t *zdict_last(zdict_t * mp)
{
    zrbtree_node_t *rn = zrbtree_last(&(mp->rbtree));
    if (rn)
    {
        return ZCONTAINER_OF(rn, zdict_node_t, rbnode);
    }
    return 0;
}

zdict_node_t *zdict_prev(zdict_node_t * node)
{
    zrbtree_node_t *rn = zrbtree_prev(&(node->rbnode));
    if (rn)
    {
        return ZCONTAINER_OF(rn, zdict_node_t, rbnode);
    }
    return 0;
}

zdict_node_t *zdict_next(zdict_node_t * node)
{
    zrbtree_node_t *rn = zrbtree_next(&(node->rbnode));
    if (rn)
    {
        return ZCONTAINER_OF(rn, zdict_node_t, rbnode);
    }
    return 0;
}

void zdict_walk(zdict_t * mp, void (*walk_fn) (zdict_node_t *, void *), void *ctx)
{
    zdict_node_t *node;

    if (!walk_fn)
    {
        return;
    }
    ZDICT_WALK_BEGIN(mp, node)
    {
        walk_fn(node, ctx);
    }
    ZDICT_WALK_END;
}

int zdict_keys(zdict_t * mp, char **key_list, int size)
{
    zrbtree_node_t *rnode;
    zdict_node_t *node;
    int i;

    i = 0;
    ZRBTREE_WALK_BEGIN(&(mp->rbtree), rnode)
    {
        node = ZCONTAINER_OF(rnode, zdict_node_t, rbnode);
        if (i == size)
        {
            goto end;
        }
        i++;
        *key_list = zdict_key(node);
        key_list++;
    }
    ZRBTREE_WALK_END;

end:

    return i;
}

