/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "zc.h"

static int zdict_zrbtree_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2)
{
    zdict_node_t *dn1, *dn2;

    dn1 = ZCONTAINER_OF(n1, zdict_node_t, rbnode);
    dn2 = ZCONTAINER_OF(n2, zdict_node_t, rbnode);

    return strcmp(dn1->key, dn2->key);
}

zdict_t *zdict_create()
{
    zdict_t *dict;
    dict = (zdict_t *) zmalloc(sizeof(zdict_t));
    dict->len = 0;
    zrbtree_init(&(dict->rbtree), zdict_zrbtree_cmp);
    return dict;
}

void zdict_free(zdict_t * dict)
{
    if (!dict) {
        return;
    }
    zdict_node_t *n;
    while ((n = zdict_first(dict))) {
        zrbtree_detach(&(dict->rbtree), &(n->rbnode));
        zbuf_fini(&(n->value));
        zfree(n->key);
        zfree(n);
    }
    zfree(dict);
}

void zdict_reset(zdict_t *dict)
{
    if (!dict) {
        return;
    }
    zdict_node_t *n;
    while ((n = zdict_first(dict))) {
        zrbtree_detach(&(dict->rbtree), &(n->rbnode));
        zbuf_fini(&(n->value));
        zfree(n->key);
        zfree(n);
    }
    dict->len = 0;
}

zdict_node_t *zdict_update_string(zdict_t * dict, const char *key, const char *value, int value_len)
{
    zdict_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    mp_n.key = (char *)(void *)key;
    rb_np = zrbtree_attach(&(dict->rbtree), &(mp_n.rbnode));

    if (value_len < 0) {
        value_len = strlen(value);
    }
    if (rb_np != &(mp_n.rbnode)) {
        mp_np = ZCONTAINER_OF(rb_np, zdict_node_t, rbnode);
        zbuf_memcpy(&(mp_np->value), value, value_len);
    } else {
        mp_np = (zdict_node_t *) zmalloc(sizeof(zdict_node_t));
        mp_np->key = zstrdup(key);
        zbuf_init(&(mp_np->value), value_len);
        zbuf_memcpy(&(mp_np->value), value, value_len);
        zrbtree_replace_node(&(dict->rbtree), &(mp_n.rbnode), &(mp_np->rbnode));
        dict->len++;
    }

    return mp_np;
}

zdict_node_t *zdict_update(zdict_t *dict, const char *key, const zbuf_t *value)
{
    zdict_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    mp_n.key = (char *)(void *)key;
    rb_np = zrbtree_attach(&(dict->rbtree), &(mp_n.rbnode));

    if (rb_np != &(mp_n.rbnode)) {
        mp_np = ZCONTAINER_OF(rb_np, zdict_node_t, rbnode);
        zbuf_memcpy(&(mp_np->value), zbuf_data(value), zbuf_len(value));
    } else {
        mp_np = (zdict_node_t *) zmalloc(sizeof(zdict_node_t));
        mp_np->key = zstrdup(key);
        zbuf_init(&(mp_np->value), zbuf_len(value));
        zbuf_memcpy(&(mp_np->value), zbuf_data(value), zbuf_len(value));
        zrbtree_replace_node(&(dict->rbtree), &(mp_n.rbnode), &(mp_np->rbnode));
        dict->len++;
    }

    return mp_np;
}

zdict_node_t *zdict_find(const zdict_t * dict, const char *key, zbuf_t **value)
{
    zdict_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value) {
        *value = 0;
    }
    mp_n.key = (char *)(void *)key;
    rb_np = zrbtree_find(&(dict->rbtree), &(mp_n.rbnode));
    if (rb_np) {
        mp_np = ZCONTAINER_OF(rb_np, zdict_node_t, rbnode);
        if (value) {
            *value = &(mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

zdict_node_t *zdict_find_near_prev(const zdict_t * dict, const char *key, zbuf_t **value)
{
    zdict_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value) {
        *value = 0;
    }

    mp_n.key = (char *)(void *)key;
    rb_np = zrbtree_near_prev(&(dict->rbtree), &(mp_n.rbnode));
    if (rb_np) {
        mp_np = ZCONTAINER_OF(rb_np, zdict_node_t, rbnode);
        if (value) {
            *value = &(mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

zdict_node_t *zdict_find_near_next(const zdict_t * dict, const char *key, zbuf_t **value)
{
    zdict_node_t mp_n, *mp_np;
    zrbtree_node_t *rb_np;

    if (value) {
        *value = 0;
    }

    mp_n.key = (char *)(void *)key;
    rb_np = zrbtree_near_next(&(dict->rbtree), &(mp_n.rbnode));
    if (rb_np) {
        mp_np = ZCONTAINER_OF(rb_np, zdict_node_t, rbnode);
        if (value) {
            *value = &(mp_np->value);
        }
        return mp_np;
    }

    return 0;
}

void zdict_delete_node(zdict_t * dict, zdict_node_t * n)
{
    zrbtree_detach(&(dict->rbtree), &(n->rbnode));
    zfree(n->key);
    zbuf_fini(&(n->value));
    zfree(n);
    dict->len--;
}

void zdict_delete(zdict_t * dict, const char *key)
{
    zdict_node_t *n;

    n = zdict_find(dict, key, 0);
    if (!n) {
        return;
    }
    zrbtree_detach(&(dict->rbtree), &(n->rbnode));
    zfree(n->key);
    zbuf_fini(&(n->value));
    zfree(n);
    dict->len--;
}

zdict_node_t *zdict_first(const zdict_t * dict)
{
    zrbtree_node_t *rn = zrbtree_first(&(dict->rbtree));
    if (rn) {
        return ZCONTAINER_OF(rn, zdict_node_t, rbnode);
    }
    return 0;
}

zdict_node_t *zdict_last(const zdict_t * dict)
{
    zrbtree_node_t *rn = zrbtree_last(&(dict->rbtree));
    if (rn) {
        return ZCONTAINER_OF(rn, zdict_node_t, rbnode);
    }
    return 0;
}

zdict_node_t *zdict_prev(const zdict_node_t * node)
{
    zrbtree_node_t *rn = zrbtree_prev(&(node->rbnode));
    if (rn) {
        return ZCONTAINER_OF(rn, zdict_node_t, rbnode);
    }
    return 0;
}

zdict_node_t *zdict_next(const zdict_node_t * node)
{
    zrbtree_node_t *rn = zrbtree_next(&(node->rbnode));
    if (rn) {
        return ZCONTAINER_OF(rn, zdict_node_t, rbnode);
    }
    return 0;
}

void zdict_debug_show(const zdict_t * dict)
{
    ZDICT_WALK_BEGIN(dict, k, v) {
        printf("%s = %s\n", k, zbuf_data(v));
    } ZDICT_WALK_END;
}

char *zdict_get_str(const zdict_t * dict, const char *name, const char *def)
{
    zbuf_t *str_val;
    if (!zdict_find(dict, name, &str_val)) {
        return (char *)(void *)def;
    }
    return zbuf_data(str_val);
}

int zdict_get_bool(const zdict_t * dict, const char *name, int def)
{
    char *str_val = zdict_get_str(dict, name, 0);
    if (str_val && *str_val) {
        return zstr_to_bool(str_val, def);
    }
    return def;
}

int zdict_get_int(const zdict_t * dict, const char *name, int def, int min, int max)
{
    int r = def;
    char *str_val = zdict_get_str(dict, name, 0);
    if (str_val && *str_val) {
        r = atoi(str_val);
        if ((r < min) || (r > max)) {
            return def;
        }
    }
    return r;
}

long zdict_get_long(const zdict_t * dict, const char *name, long def, long min, long max)
{
    long r = def;
    char *str_val = zdict_get_str(dict, name, 0);
    if (str_val && *str_val) {
        r = atol(str_val);
        if ((r < min) || (r > max)) {
            return def;
        }
    }
    return r;
}

long zdict_get_second(const zdict_t *dict, const char *name, long def, long min, long max)
{
    long r = def;
    char *str_val = zdict_get_str(dict, name, 0);
    if (str_val && *str_val) {
        r = zstr_to_second(str_val, def);
        if ((r < min) || (r > max)) {
            return def;
        }
    }
    return r;
}

long zdict_get_size(const zdict_t *dict, const char *name, long def, long min, long max)
{
    long r = def;
    char *str_val = zdict_get_str(dict, name, 0);
    if (str_val && *str_val) {
        r = zstr_to_size(str_val, def);
        if ((r < min) || (r > max)) {
            return def;
        }
    }
    return r;
}
