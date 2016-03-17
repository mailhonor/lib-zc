/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "libzc.h"

static int zgrid_rbtree_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2)
{
    zgrid_node_t *dn1, *dn2;

    dn1 = ZCONTAINER_OF(n1, zgrid_node_t, rbnode);
    dn2 = ZCONTAINER_OF(n2, zgrid_node_t, rbnode);

    return strcmp(dn1->key, dn2->key);
}

zgrid_t *zgrid_create(void)
{
    return zgrid_create_mpool(0);
}

zgrid_t *zgrid_create_mpool(zmpool_t *mpool)
{
    zgrid_t *mp;

    mp = (zgrid_t *) zmpool_malloc(mpool, sizeof(zgrid_t));
    mp->mpool = mpool;
    mp->len = 0;
    zrbtree_init(&(mp->rbtree), zgrid_rbtree_cmp);

    return mp;
}

void zgrid_free(zgrid_t * mp, void (*free_fn) (zgrid_node_t *, void *), void *ctx)
{
    zgrid_node_t *n;

    while ((n = zgrid_first(mp)))
    {
        zrbtree_detach(&(mp->rbtree), &(n->rbnode));
        if (free_fn)
        {
            (*free_fn) (n, ctx);
        }
         zmpool_free(mp->mpool, n->key);
         zmpool_free(mp->mpool, n);
    }
    zmpool_free(mp->mpool, mp);
}

zgrid_node_t *zgrid_add(zgrid_t * mp, char *key, void *value, char **old_value)
{
    zgrid_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (old_value)
    {
        *old_value = 0;
    }
    mp_n.key = key;
    rb_np = zrbtree_attach(&(mp->rbtree), &(mp_n.rbnode));

    if (rb_np != &(mp_n.rbnode))
    {
        mp_np = ZCONTAINER_OF(rb_np, zgrid_node_t, rbnode);
        if (old_value)
        {
            *old_value = (char *)(mp_np->value);
        }
        mp_np->value = value;
    }
    else
    {
        mp_np = (zgrid_node_t *) zmpool_malloc(mp->mpool, sizeof(zgrid_node_t));
        mp_np->key = zmpool_strdup(mp->mpool, key);
        mp_np->value = value;
        zrbtree_replace_node(&(mp->rbtree), &(mp_n.rbnode), &(mp_np->rbnode));
        mp->len ++;
    }

    return mp_np;
}

zgrid_node_t *zgrid_lookup(zgrid_t * mp, char *key, char **value)
{
    zgrid_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value)
    {
        *value = 0;
    }
    mp_n.key = key;
    rb_np = zrbtree_lookup(&(mp->rbtree), &(mp_n.rbnode));
    if (rb_np)
    {
        mp_np = ZCONTAINER_OF(rb_np, zgrid_node_t, rbnode);
        if (value)
        {
            *value = (char *)(mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

zgrid_node_t *zgrid_lookup_near_prev(zgrid_t * mp, char *key, char **value)
{
    zgrid_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value)
    {
        *value = 0;
    }

    mp_n.key = key;
    rb_np = zrbtree_near_prev(&(mp->rbtree), &(mp_n.rbnode));
    if (rb_np)
    {
        mp_np = ZCONTAINER_OF(rb_np, zgrid_node_t, rbnode);
        if (value)
        {
            *value = (char *)(mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

zgrid_node_t *zgrid_lookup_near_next(zgrid_t * mp, char *key, char **value)
{
    zgrid_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value)
    {
        *value = 0;
    }

    mp_n.key = key;
    rb_np = zrbtree_near_next(&(mp->rbtree), &(mp_n.rbnode));
    if (rb_np)
    {
        mp_np = ZCONTAINER_OF(rb_np, zgrid_node_t, rbnode);
        if (value)
        {
            *value = (char *)(mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

void zgrid_delete_node(zgrid_t * mp, zgrid_node_t *n)
{
    zrbtree_detach(&(mp->rbtree), &(n->rbnode));
         zmpool_free(mp->mpool, n->key);
    zmpool_free(mp->mpool, n);
    mp->len --;
}

void zgrid_delete(zgrid_t * mp, char *key, char **old_value)
{
    zgrid_node_t *n;

    n = zgrid_lookup(mp, key, old_value);
    if (!n)
    {
        return;
    }
    zrbtree_detach(&(mp->rbtree), &(n->rbnode));
    zmpool_free(mp->mpool, n->key);
    zmpool_free(mp->mpool, n);
    mp->len --;
}

zgrid_node_t *zgrid_first(zgrid_t * mp)
{
    zrbtree_node_t *rn = zrbtree_first(&(mp->rbtree));
    if (rn)
    {
        return ZCONTAINER_OF(rn, zgrid_node_t, rbnode);
    }
    return 0;
}

zgrid_node_t *zgrid_last(zgrid_t * mp)
{
    zrbtree_node_t *rn = zrbtree_last(&(mp->rbtree));
    if (rn)
    {
        return ZCONTAINER_OF(rn, zgrid_node_t, rbnode);
    }
    return 0;
}

zgrid_node_t *zgrid_prev(zgrid_node_t * node)
{
    zrbtree_node_t *rn = zrbtree_prev(&(node->rbnode));
    if (rn)
    {
        return ZCONTAINER_OF(rn, zgrid_node_t, rbnode);
    }
    return 0;
}

zgrid_node_t *zgrid_next(zgrid_node_t * node)
{
    zrbtree_node_t *rn = zrbtree_next(&(node->rbnode));
    if (rn)
    {
        return ZCONTAINER_OF(rn, zgrid_node_t, rbnode);
    }
    return 0;
}

void zgrid_walk(zgrid_t * mp, void (*walk_fn) (zgrid_node_t *, void *), void *ctx)
{
    zgrid_node_t *node;

    if (!walk_fn)
    {
        return;
    }
    ZGRID_WALK_BEGIN(mp, node)
    {
        walk_fn(node, ctx);
    }
    ZGRID_WALK_END;
}

int zgrid_keys(zgrid_t * mp, char **key_list, int size)
{
    zrbtree_node_t *rnode;
    zgrid_node_t *node;
    int i;

    i = 0;
    ZRBTREE_WALK_BEGIN(&(mp->rbtree), rnode)
    {
        node = ZCONTAINER_OF(rnode, zgrid_node_t, rbnode);
        if (i == size)
        {
            goto end;
        }
        i++;
        *key_list = zgrid_key(node);
        key_list++;
    }
    ZRBTREE_WALK_END;

end:

    return i;
}

