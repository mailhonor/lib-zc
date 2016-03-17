/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "libzc.h"

static int zigrid_rbtree_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2)
{
    zigrid_node_t *dn1, *dn2;

    dn1 = ZCONTAINER_OF(n1, zigrid_node_t, rbnode);
    dn2 = ZCONTAINER_OF(n2, zigrid_node_t, rbnode);

    return (dn1->key - dn2->key);
}

zigrid_t *zigrid_create(void)
{
    return zigrid_create_mpool(0);
}

zigrid_t *zigrid_create_mpool(zmpool_t *mpool)
{
    zigrid_t *mp;

    mp = (zigrid_t *) zmpool_malloc(mpool, sizeof(zigrid_t));
    mp->mpool = mpool;
    mp->len = 0;
    zrbtree_init(&(mp->rbtree), zigrid_rbtree_cmp);

    return mp;
}

void zigrid_free(zigrid_t * mp, void (*free_fn) (zigrid_node_t *, void *), void *ctx)
{
    zigrid_node_t *n;

    while ((n = zigrid_first(mp)))
    {
        zrbtree_detach(&(mp->rbtree), &(n->rbnode));
        if (free_fn)
        {
            (*free_fn) (n, ctx);
        }
         zmpool_free(mp->mpool, n);
    }
    zmpool_free(mp->mpool, mp);
}

zigrid_node_t *zigrid_add(zigrid_t * mp, long key, void *value, char **old_value)
{
    zigrid_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (old_value)
    {
        *old_value = 0;
    }
    mp_n.key = key;
    rb_np = zrbtree_attach(&(mp->rbtree), &(mp_n.rbnode));

    if (rb_np != &(mp_n.rbnode))
    {
        mp_np = ZCONTAINER_OF(rb_np, zigrid_node_t, rbnode);
        if (old_value)
        {
            *old_value = (char *)(mp_np->value);
        }
        mp_np->value = value;
    }
    else
    {
        mp_np = (zigrid_node_t *) zmpool_malloc(mp->mpool, sizeof(zigrid_node_t));
        mp_np->key = key;
        mp_np->value = value;
        zrbtree_replace_node(&(mp->rbtree), &(mp_n.rbnode), &(mp_np->rbnode));
        mp->len ++;
    }

    return mp_np;
}

zigrid_node_t *zigrid_lookup(zigrid_t * mp, long key, char **value)
{
    zigrid_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value)
    {
        *value = 0;
    }
    mp_n.key = key;
    rb_np = zrbtree_lookup(&(mp->rbtree), &(mp_n.rbnode));
    if (rb_np)
    {
        mp_np = ZCONTAINER_OF(rb_np, zigrid_node_t, rbnode);
        if (value)
        {
            *value = (char *)(mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

zigrid_node_t *zigrid_lookup_near_prev(zigrid_t * mp, long key, char **value)
{
    zigrid_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value)
    {
        *value = 0;
    }

    mp_n.key = key;
    rb_np = zrbtree_near_prev(&(mp->rbtree), &(mp_n.rbnode));
    if (rb_np)
    {
        mp_np = ZCONTAINER_OF(rb_np, zigrid_node_t, rbnode);
        if (value)
        {
            *value = (char *)(mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

zigrid_node_t *zigrid_lookup_near_next(zigrid_t * mp, long key, char **value)
{
    zigrid_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value)
    {
        *value = 0;
    }

    mp_n.key = key;
    rb_np = zrbtree_near_next(&(mp->rbtree), &(mp_n.rbnode));
    if (rb_np)
    {
        mp_np = ZCONTAINER_OF(rb_np, zigrid_node_t, rbnode);
        if (value)
        {
            *value = (char *)(mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

void zigrid_delete_node(zigrid_t * mp, zigrid_node_t *n)
{
    zrbtree_detach(&(mp->rbtree), &(n->rbnode));
    zmpool_free(mp->mpool, n);
    mp->len --;
}

void zigrid_delete(zigrid_t * mp, long key, char **old_value)
{
    zigrid_node_t *n;

    n = zigrid_lookup(mp, key, old_value);
    if (!n)
    {
        return;
    }
    zrbtree_detach(&(mp->rbtree), &(n->rbnode));
    zmpool_free(mp->mpool, n);
    mp->len --;
}

zigrid_node_t *zigrid_first(zigrid_t * mp)
{
    zrbtree_node_t *rn = zrbtree_first(&(mp->rbtree));
    if (rn)
    {
        return ZCONTAINER_OF(rn, zigrid_node_t, rbnode);
    }
    return 0;
}

zigrid_node_t *zigrid_last(zigrid_t * mp)
{
    zrbtree_node_t *rn = zrbtree_last(&(mp->rbtree));
    if (rn)
    {
        return ZCONTAINER_OF(rn, zigrid_node_t, rbnode);
    }
    return 0;
}

zigrid_node_t *zigrid_prev(zigrid_node_t * node)
{
    zrbtree_node_t *rn = zrbtree_prev(&(node->rbnode));
    if (rn)
    {
        return ZCONTAINER_OF(rn, zigrid_node_t, rbnode);
    }
    return 0;
}

zigrid_node_t *zigrid_next(zigrid_node_t * node)
{
    zrbtree_node_t *rn = zrbtree_next(&(node->rbnode));
    if (rn)
    {
        return ZCONTAINER_OF(rn, zigrid_node_t, rbnode);
    }
    return 0;
}

void zigrid_walk(zigrid_t * mp, void (*walk_fn) (zigrid_node_t *, void *), void *ctx)
{
    zigrid_node_t *node;

    if (!walk_fn)
    {
        return;
    }
    ZIGRID_WALK_BEGIN(mp, node)
    {
        walk_fn(node, ctx);
    }
    ZIGRID_WALK_END;
}

int zigrid_keys(zigrid_t * mp, long *key_list, int size)
{
    zrbtree_node_t *rnode;
    zigrid_node_t *node;
    int i;

    i = 0;
    ZRBTREE_WALK_BEGIN(&(mp->rbtree), rnode)
    {
        node = ZCONTAINER_OF(rnode, zigrid_node_t, rbnode);
        if (i == size)
        {
            goto end;
        }
        i++;
        *key_list = zigrid_key(node);
        key_list++;
    }
    ZRBTREE_WALK_END;

end:

    return i;
}

