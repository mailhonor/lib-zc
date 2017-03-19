/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "zc.h"

static int zidict_rbtree_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2)
{
    zidict_node_t *dn1, *dn2;

    dn1 = ZCONTAINER_OF(n1, zidict_node_t, rbnode);
    dn2 = ZCONTAINER_OF(n2, zidict_node_t, rbnode);

    return (dn1->key - dn2->key);
}

zidict_t *zidict_create(void)
{
    return zidict_create_mpool(0);
}

zidict_t *zidict_create_mpool(zmpool_t * mpool)
{
    zidict_t *mp;

    mp = (zidict_t *) zmpool_malloc(mpool, sizeof(zidict_t));
    mp->mpool = mpool;
    mp->len = 0;
    mp->is_STR = 0;
    zrbtree_init(&(mp->rbtree), zidict_rbtree_cmp);

    return mp;
}

zidict_t *zidict_create_STR(void)
{
    return zidict_create_mpool_STR(0);
}

zidict_t *zidict_create_mpool_STR(zmpool_t * mpool)
{
    zidict_t *mp;

    mp = (zidict_t *) zmpool_malloc(mpool, sizeof(zidict_t));
    mp->mpool = mpool;
    mp->len = 0;
    mp->is_STR = 1;
    zrbtree_init(&(mp->rbtree), zidict_rbtree_cmp);

    return mp;
}

void zidict_free(zidict_t * mp)
{
    zidict_node_t *n;

    while ((n = zidict_first(mp))) {
        zrbtree_detach(&(mp->rbtree), &(n->rbnode));
        if (mp->is_STR) {
            zmpool_free(mp->mpool, n->value);
        }
        zmpool_free(mp->mpool, n);
    }
    zmpool_free(mp->mpool, mp);
}

zidict_node_t *zidict_update(zidict_t * mp, long key, const void *value, char **old_value)
{
    zidict_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    mp_n.key = key;
    rb_np = zrbtree_attach(&(mp->rbtree), &(mp_n.rbnode));

    if (rb_np != &(mp_n.rbnode)) {
        mp_np = ZCONTAINER_OF(rb_np, zidict_node_t, rbnode);
        if (old_value) {
            *old_value = (char *)(mp_np->value);
        }
        mp_np->value = (char *)value;
    } else {
        mp_np = (zidict_node_t *) zmpool_malloc(mp->mpool, sizeof(zidict_node_t));
        mp_np->key = key;
        if (old_value) {
            *old_value = (char *)(mp_np->value);
        }
        mp_np->value = (char *)value;
        zrbtree_replace_node(&(mp->rbtree), &(mp_n.rbnode), &(mp_np->rbnode));
        mp->len++;
    }

    return mp_np;
}

zidict_node_t *zidict_update_STR(zidict_t * mp, long key, const char *value)
{
    zidict_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    mp_n.key = key;
    rb_np = zrbtree_attach(&(mp->rbtree), &(mp_n.rbnode));

    if (rb_np != &(mp_n.rbnode)) {
        mp_np = ZCONTAINER_OF(rb_np, zidict_node_t, rbnode);
        zmpool_free(mp->mpool, mp_np->value);
        mp_np->value = zmpool_strdup(mp->mpool, value);
    } else {
        mp_np = (zidict_node_t *) zmpool_malloc(mp->mpool, sizeof(zidict_node_t));
        mp_np->key = key;
        mp_np->value = zmpool_strdup(mp->mpool, value);
        zrbtree_replace_node(&(mp->rbtree), &(mp_n.rbnode), &(mp_np->rbnode));
        mp->len++;
    }

    return mp_np;
}

zidict_node_t *zidict_find(zidict_t * mp, long key, char **value)
{
    zidict_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value) {
        *value = 0;
    }
    mp_n.key = key;
    rb_np = zrbtree_find(&(mp->rbtree), &(mp_n.rbnode));
    if (rb_np) {
        mp_np = ZCONTAINER_OF(rb_np, zidict_node_t, rbnode);
        if (value) {
            *value = (char *)(mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

zidict_node_t *zidict_find_near_prev(zidict_t * mp, long key, char **value)
{
    zidict_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value) {
        *value = 0;
    }

    mp_n.key = key;
    rb_np = zrbtree_near_prev(&(mp->rbtree), &(mp_n.rbnode));
    if (rb_np) {
        mp_np = ZCONTAINER_OF(rb_np, zidict_node_t, rbnode);
        if (value) {
            *value = (char *)(mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

zidict_node_t *zidict_find_near_next(zidict_t * mp, long key, char **value)
{
    zidict_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value) {
        *value = 0;
    }

    mp_n.key = key;
    rb_np = zrbtree_near_next(&(mp->rbtree), &(mp_n.rbnode));
    if (rb_np) {
        mp_np = ZCONTAINER_OF(rb_np, zidict_node_t, rbnode);
        if (value) {
            *value = (char *)(mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

void zidict_erase_node(zidict_t * mp, zidict_node_t * n)
{
    zrbtree_detach(&(mp->rbtree), &(n->rbnode));
    if (mp->is_STR) {
        zmpool_free(mp->mpool, n->value);
    }
    zmpool_free(mp->mpool, n);
    mp->len--;
}

void zidict_erase(zidict_t * mp, long key, char **old_value)
{
    zidict_node_t *n;

    n = zidict_find(mp, key, 0);
    if (!n) {
        return;
    }
    zrbtree_detach(&(mp->rbtree), &(n->rbnode));
    if (old_value) {
        *old_value = (char *)(n->value);
    }
    zmpool_free(mp->mpool, n);
    mp->len--;
}

void zidict_erase_STR(zidict_t * mp, long key)
{
    zidict_node_t *n;

    n = zidict_find(mp, key, 0);
    if (!n) {
        return;
    }
    zrbtree_detach(&(mp->rbtree), &(n->rbnode));
    zmpool_free(mp->mpool, n->value);
    zmpool_free(mp->mpool, n);
    mp->len--;
}

zidict_node_t *zidict_first(zidict_t * mp)
{
    zrbtree_node_t *rn = zrbtree_first(&(mp->rbtree));
    if (rn) {
        return ZCONTAINER_OF(rn, zidict_node_t, rbnode);
    }
    return 0;
}

zidict_node_t *zidict_last(zidict_t * mp)
{
    zrbtree_node_t *rn = zrbtree_last(&(mp->rbtree));
    if (rn) {
        return ZCONTAINER_OF(rn, zidict_node_t, rbnode);
    }
    return 0;
}

zidict_node_t *zidict_prev(zidict_node_t * node)
{
    zrbtree_node_t *rn = zrbtree_prev(&(node->rbnode));
    if (rn) {
        return ZCONTAINER_OF(rn, zidict_node_t, rbnode);
    }
    return 0;
}

zidict_node_t *zidict_next(zidict_node_t * node)
{
    zrbtree_node_t *rn = zrbtree_next(&(node->rbnode));
    if (rn) {
        return ZCONTAINER_OF(rn, zidict_node_t, rbnode);
    }
    return 0;
}

int zidict_keys(zidict_t * mp, long *key_list, int size)
{
    zrbtree_node_t *rnode;
    zidict_node_t *node;
    int i;

    i = 0;
    ZRBTREE_WALK_BEGIN(&(mp->rbtree), rnode) {
        node = ZCONTAINER_OF(rnode, zidict_node_t, rbnode);
        if (i == size) {
            goto end;
        }
        i++;
        *key_list = ZIDICT_KEY(node);
        key_list++;
    }
    ZRBTREE_WALK_END;

  end:

    return i;
}
