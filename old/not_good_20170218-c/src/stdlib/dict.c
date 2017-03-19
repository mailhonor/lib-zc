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

zdict_t *zdict_create_mpool(zmpool_t * mpool)
{
    zdict_t *dict;

    dict = (zdict_t *) zmpool_malloc(mpool, sizeof(zdict_t));
    dict->mpool = mpool;
    dict->len = 0;
    zrbtree_init(&(dict->rbtree), zdict_rbtree_cmp);

    return dict;
}

void zdict_free(zdict_t * dict)
{
    zdict_node_t *n;

    while ((n = zdict_first(dict))) {
        zrbtree_detach(&(dict->rbtree), &(n->rbnode));
        zmpool_free(dict->mpool, n->value);
        zmpool_free(dict->mpool, n->key);
        zmpool_free(dict->mpool, n);
    }
    zmpool_free(dict->mpool, dict);
}

zdict_node_t *zdict_add(zdict_t * dict, const char *key, const char *value)
{
    zdict_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    mp_n.key = (char *)key;
    rb_np = zrbtree_attach(&(dict->rbtree), &(mp_n.rbnode));

    if (rb_np != &(mp_n.rbnode)) {
        mp_np = ZCONTAINER_OF(rb_np, zdict_node_t, rbnode);
        zmpool_free(dict->mpool, mp_np->value);
        mp_np->value = zmpool_strdup(dict->mpool, value);
    } else {
        mp_np = (zdict_node_t *) zmpool_malloc(dict->mpool, sizeof(zdict_node_t));
        mp_np->key = zmpool_strdup(dict->mpool, key);
        mp_np->value = zmpool_strdup(dict->mpool, value);
        zrbtree_replace_node(&(dict->rbtree), &(mp_n.rbnode), &(mp_np->rbnode));
        dict->len++;
    }

    return mp_np;
}

zdict_node_t *zdict_lookup(zdict_t * dict, const char *key, char **value)
{
    zdict_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value) {
        *value = 0;
    }
    mp_n.key = (char *)key;
    rb_np = zrbtree_lookup(&(dict->rbtree), &(mp_n.rbnode));
    if (rb_np) {
        mp_np = ZCONTAINER_OF(rb_np, zdict_node_t, rbnode);
        if (value) {
            *value = (char *)(mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

zdict_node_t *zdict_lookup_near_prev(zdict_t * dict, const char *key, char **value)
{
    zdict_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value) {
        *value = 0;
    }

    mp_n.key = (char *)key;
    rb_np = zrbtree_near_prev(&(dict->rbtree), &(mp_n.rbnode));
    if (rb_np) {
        mp_np = ZCONTAINER_OF(rb_np, zdict_node_t, rbnode);
        if (value) {
            *value = (char *)(mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

zdict_node_t *zdict_lookup_near_next(zdict_t * dict, const char *key, char **value)
{
    zdict_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value) {
        *value = 0;
    }

    mp_n.key = (char *)key;
    rb_np = zrbtree_near_next(&(dict->rbtree), &(mp_n.rbnode));
    if (rb_np) {
        mp_np = ZCONTAINER_OF(rb_np, zdict_node_t, rbnode);
        if (value) {
            *value = (char *)(mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

void zdict_delete_node(zdict_t * dict, zdict_node_t * n)
{
    zrbtree_detach(&(dict->rbtree), &(n->rbnode));
    zmpool_free(dict->mpool, n->key);
    zmpool_free(dict->mpool, n->value);
    zmpool_free(dict->mpool, n);
    dict->len--;
}

void zdict_delete(zdict_t * dict, const char *key)
{
    zdict_node_t *n;

    n = zdict_lookup(dict, key, 0);
    if (!n) {
        return;
    }
    zrbtree_detach(&(dict->rbtree), &(n->rbnode));
    zmpool_free(dict->mpool, n->key);
    zmpool_free(dict->mpool, n->value);
    zmpool_free(dict->mpool, n);
    dict->len--;
}

zdict_node_t *zdict_first(zdict_t * dict)
{
    zrbtree_node_t *rn = zrbtree_first(&(dict->rbtree));
    if (rn) {
        return ZCONTAINER_OF(rn, zdict_node_t, rbnode);
    }
    return 0;
}

zdict_node_t *zdict_last(zdict_t * dict)
{
    zrbtree_node_t *rn = zrbtree_last(&(dict->rbtree));
    if (rn) {
        return ZCONTAINER_OF(rn, zdict_node_t, rbnode);
    }
    return 0;
}

zdict_node_t *zdict_prev(zdict_node_t * node)
{
    zrbtree_node_t *rn = zrbtree_prev(&(node->rbnode));
    if (rn) {
        return ZCONTAINER_OF(rn, zdict_node_t, rbnode);
    }
    return 0;
}

zdict_node_t *zdict_next(zdict_node_t * node)
{
    zrbtree_node_t *rn = zrbtree_next(&(node->rbnode));
    if (rn) {
        return ZCONTAINER_OF(rn, zdict_node_t, rbnode);
    }
    return 0;
}

int zdict_keys(zdict_t * dict, char **key_list, int size)
{
    zrbtree_node_t *rnode;
    zdict_node_t *node;
    int i;

    i = 0;
    ZRBTREE_WALK_BEGIN(&(dict->rbtree), rnode) {
        node = ZCONTAINER_OF(rnode, zdict_node_t, rbnode);
        if (i == size) {
            goto end;
        }
        i++;
        *key_list = (char *)ZDICT_KEY(node);
        key_list++;
    }
    ZRBTREE_WALK_END;

  end:

    return i;
}

void zdict_show(zdict_t * dict)
{
    zdict_node_t *n;

    ZDICT_WALK_BEGIN(dict, n) {
        printf("%s = %s\n", ZDICT_KEY(n), ZDICT_VALUE(n));
    } ZDICT_WALK_END;
}

