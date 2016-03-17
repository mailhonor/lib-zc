/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-09-28
 * ================================
 */

#include "libzc.h"

#define zhtable_link(table, elm)\
{ \
    zhtable_node_t **_h = table->data + (*(table->hash_fn))(elm, table->size);\
    elm->prev = 0; \
    if ((elm->next = *_h) != 0) \
    (*_h)->prev = elm; \
    *_h = elm; \
    table->len++; \
}

static void zhtable_size(zhtable_t * table, unsigned int size)
{
    size |= 1;
    table->data = (zhtable_node_t **) zcalloc(sizeof(zhtable_node_t *), size);
    table->size = size;
    table->len = 0;
}

static int zhtable_grow(zhtable_t * table)
{
    zhtable_node_t *ht;
    zhtable_node_t *next;
    unsigned old_size = table->size;
    zhtable_node_t **h = table->data;
    zhtable_node_t **old_entries = h;

    zhtable_size(table, 2 * old_size);

    while (old_size-- > 0)
    {
        for (ht = *h++; ht; ht = next)
        {
            next = ht->next;
            zhtable_link(table, ht);
        }
    }
    zfree((char *)old_entries);

    return 1;
}

void zhtable_init(zhtable_t * table, int size, zhtable_cmp_t cmp_fn, zhtable_hash_t hash_fn)
{
    zhtable_size(table, size < 13 ? 13 : size);
    table->cmp_fn = cmp_fn;
    table->hash_fn = hash_fn;
}

zhtable_node_t *zhtable_attach(zhtable_t * table, zhtable_node_t * node)
{
    if (table->len >= table->size)
    {
        zhtable_grow(table);
    }
    zhtable_link(table, node);

    return (node);
}

zhtable_node_t *zhtable_lookup(zhtable_t * table, zhtable_node_t * vnode)
{
    zhtable_node_t *ht;
    for (ht = table->data[table->hash_fn(vnode, table->size)]; ht; ht = ht->next)
    {
        if (!table->cmp_fn(vnode, ht))
        {
            return (ht);
        }
    }
    return (0);
}

zhtable_node_t *zhtable_remove(zhtable_t * table, zhtable_node_t * vnode)
{
    zhtable_node_t *ht;
    zhtable_node_t **h = table->data + table->hash_fn(vnode, table->size);

    for (ht = *h; ht; ht = ht->next)
    {
        if (table->cmp_fn(vnode, ht))
        {
            if (ht->next)
            {
                ht->next->prev = ht->prev;
            }
            if (ht->prev)
            {
                ht->prev->next = ht->next;
            }
            else
            {
                *h = ht->next;
            }
            table->len--;
            return ht;
        }
    }
    return 0;
}

zhtable_node_t *zhtable_detach(zhtable_t * table, zhtable_node_t * node)
{
    if (node->next)
        node->next->prev = node->prev;
    if (node->prev)
        node->prev->next = node->next;
    table->len--;
    return node;
}

void zhtable_fini(zhtable_t * table, void (*fini_fn) (zhtable_node_t *, void *), void *ctx)
{
    unsigned i = table->size;
    zhtable_node_t *ht;
    zhtable_node_t *next;
    zhtable_node_t **h = table->data;

    while (i-- > 0)
    {
        for (ht = *h++; ht; ht = next)
        {
            next = ht->next;
            if (fini_fn)
            {
                (*fini_fn) (ht, ctx);
            }
        }
    }
    zfree((char *)table->data);
}

void zhtable_walk(zhtable_t * table, void (*walk_fn) (zhtable_node_t *, void *), void *ctx)
{
    unsigned i = table->size;
    zhtable_node_t **h = table->data;
    zhtable_node_t *ht;
    if (!walk_fn)
    {
        return;
    }
    while (i-- > 0)
    {
        for (ht = *h++; ht; ht = ht->next)
        {
            (*walk_fn) (ht, ctx);
        }
    }
}

zhtable_node_t **zhtable_list(zhtable_t * table, zhtable_node_t ** list)
{
    zhtable_node_t *member;
    int count = 0;
    int i;

    if (!list)
    {
        list = (zhtable_node_t **) zmalloc(sizeof(*list) * (table->len + 1));
    }
    for (i = 0; i < table->size; i++)
        for (member = table->data[i]; member != 0; member = member->next)
            list[count++] = member;
    list[count] = 0;

    return (list);
}
