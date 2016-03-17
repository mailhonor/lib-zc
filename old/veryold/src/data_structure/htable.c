#include "zc.h"

#define zhtable_link(table, elm)\
{ \
	ZHTABLE_NODE **_h = table->data + (*(table->hash_fn))(elm, table->size);\
	elm->prev = 0; \
	if ((elm->next = *_h) != 0) \
	(*_h)->prev = elm; \
	*_h = elm; \
	table->len++; \
}

static void zhtable_size(ZHTABLE * table, unsigned int size)
{
	size |= 1;
	table->data = (ZHTABLE_NODE **) zcalloc(sizeof(ZHTABLE_NODE *), size);
	table->size = size;
	table->len = 0;
}

static int zhtable_grow(ZHTABLE * table)
{
	ZHTABLE_NODE *ht;
	ZHTABLE_NODE *next;
	unsigned old_size = table->size;
	ZHTABLE_NODE **h = table->data;
	ZHTABLE_NODE **old_entries = h;

	zhtable_size(table, 2 * old_size);

	while (old_size-- > 0) {
		for (ht = *h++; ht; ht = next) {
			next = ht->next;
			zhtable_link(table, ht);
		}
	}
	zfree((char *)old_entries);

	return 1;
}

void zhtable_init(ZHTABLE * table, int size, ZHTABLE_CMP_FN cmp_fn, ZHTABLE_HASH_FN hash_fn)
{
	zhtable_size(table, size < 13 ? 13 : size);
	table->cmp_fn = cmp_fn;
	table->hash_fn = hash_fn;
}

ZHTABLE_NODE *zhtable_attach(ZHTABLE * table, ZHTABLE_NODE * node)
{
	if (table->len >= table->size) {
		zhtable_grow(table);
	}
	zhtable_link(table, node);

	return (node);
}

ZHTABLE_NODE *zhtable_lookup(ZHTABLE * table, ZHTABLE_NODE * vnode)
{
	ZHTABLE_NODE *ht;
	for (ht = table->data[table->hash_fn(vnode, table->size)]; ht; ht = ht->next) {
		if (!table->cmp_fn(vnode, ht)) {
			return (ht);
		}
	}
	return (0);
}

ZHTABLE_NODE *zhtable_remove(ZHTABLE * table, ZHTABLE_NODE * vnode)
{
	ZHTABLE_NODE *ht;
	ZHTABLE_NODE **h = table->data + table->hash_fn(vnode, table->size);

	for (ht = *h; ht; ht = ht->next) {
		if (table->cmp_fn(vnode, ht)) {
			if (ht->next) {
				ht->next->prev = ht->prev;
			}
			if (ht->prev) {
				ht->prev->next = ht->next;
			} else {
				*h = ht->next;
			}
			table->len--;
			return ht;
		}
	}
	return 0;
}

ZHTABLE_NODE *zhtable_detach(ZHTABLE * table, ZHTABLE_NODE * node)
{
	if (node->next)
		node->next->prev = node->prev;
	if (node->prev)
		node->prev->next = node->next;
	table->len--;
	return node;
}

void zhtable_fini(ZHTABLE * table, void (*fini_fn) (ZHTABLE_NODE *, void *), void *ctx)
{
	unsigned i = table->size;
	ZHTABLE_NODE *ht;
	ZHTABLE_NODE *next;
	ZHTABLE_NODE **h = table->data;

	while (i-- > 0) {
		for (ht = *h++; ht; ht = next) {
			next = ht->next;
			if (fini_fn) {
				(*fini_fn) (ht, ctx);
			}
		}
	}
	zfree((char *)table->data);
}

void zhtable_walk(ZHTABLE * table, void (*walk_fn) (ZHTABLE_NODE *, void *), void *ctx)
{
	unsigned i = table->size;
	ZHTABLE_NODE **h = table->data;
	ZHTABLE_NODE *ht;
	if (!walk_fn) {
		return;
	}
	while (i-- > 0) {
		for (ht = *h++; ht; ht = ht->next) {
			(*walk_fn) (ht, ctx);
		}
	}
}

ZHTABLE_NODE **zhtable_list(ZHTABLE * table, ZHTABLE_NODE ** list)
{
	ZHTABLE_NODE *member;
	int count = 0;
	int i;

	if (!list) {
		list = (ZHTABLE_NODE **) zmalloc(sizeof(*list) * (table->len + 1));
	}
	for (i = 0; i < table->size; i++)
		for (member = table->data[i]; member != 0; member = member->next)
			list[count++] = member;
	list[count] = 0;

	return (list);
}
