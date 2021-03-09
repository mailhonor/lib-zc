/*
 * please notice clib-inner funcs bsearch, hsearch, lsearch, qsort etc.
 */
#include "zyc.h"

#define	KEY_EQ(x,y) (x[0] == y[0] && strcmp(x,y) == 0)

#define zhash_link(table, elm) { \
	ZHASH_NODE **_h = table->data + zhash_hash(elm->key, table->size);\
	elm->prev = 0; \
	if ((elm->next = *_h) != 0) \
	(*_h)->prev = elm; \
	*_h = elm; \
	table->len++; \
}

static unsigned zhash_hash(const char *key, unsigned size) {
	unsigned long h = 0;
	unsigned long g;
	while (*key) {
		h = (h << 4U) + *key++;
		if ((g = (h & 0xf0000000)) != 0) {
			h ^= (g >> 24U);
			h ^= g;
		}
	}
	return (h % size);
}

static void zhash_size(ZHASH *table, unsigned size){
	ZHASH_NODE **h;
	size |= 1;
	table->data = h = (ZHASH_NODE **) z_malloc(size * sizeof(ZHASH_NODE *));
	table->size = size;
	table->len = 0;
	while (size-- > 0)
		*h++ = 0;
}

ZHASH *zhash_create(int size){
	ZHASH *table;

	table = (ZHASH *) z_malloc(sizeof(ZHASH));
	zhash_size(table, size < 13 ? 13 : size);
	return (table);
}

static int zhash_grow(ZHASH *table){
	ZHASH_NODE *ht;
	ZHASH_NODE *next;
	unsigned old_size = table->size;
	ZHASH_NODE **h = table->data;
	ZHASH_NODE **old_entries = h;

	zhash_size(table, 2 * old_size);

	while (old_size-- > 0) {
		for (ht = *h++; ht; ht = next) {
			next = ht->next;
			zhash_link(table, ht);
		}
	}
	z_free((char *) old_entries);
	return 1;
}

ZHASH_NODE *zhash_enter(ZHASH *table, const char *key, char *value){
	ZHASH_NODE *ht;

	if (table->len >= table->size)
		zhash_grow(table);
	ht = (ZHASH_NODE *) z_malloc(sizeof(ZHASH_NODE));
	ht->key = z_strdup(key);
	ht->value = value;
	zhash_link(table, ht);
	return (ht);
}

ZHASH_NODE *zhash_enter_unique(ZHASH *table, const char *key, char *value, char **old_value){
	ZHASH_NODE *ht;

	if (table->len >= table->size)
		zhash_grow(table);

	if(old_value){
		*old_value=0;
	}

	for (ht = table->data[zhash_hash(key, table->size)]; ht; ht = ht->next){
		if (KEY_EQ(key, ht->key)){
			if(old_value){
				*old_value=ht->value;
			}
			ht->value=value;
			return (ht);
		}
	}

	ht = (ZHASH_NODE *) z_malloc(sizeof(ZHASH_NODE));
	ht->key = z_strdup(key);
	ht->value = value;
	zhash_link(table, ht);
	return (ht);
}

ZHASH_NODE *zhash_find(ZHASH *table, const char *key, char **value){
	ZHASH_NODE *ht;

	for (ht = table->data[zhash_hash(key, table->size)]; ht; ht = ht->next){
		if (KEY_EQ(key, ht->key)){
			if(value){
				*value=ht->value;
			}
			return (ht);
		}
	}
	return (0);
}

ZHASH_NODE *zhash_delete(ZHASH *table, const char *key, char **value){
	ZHASH_NODE *ht;
	ZHASH_NODE **h = table->data + zhash_hash(key, table->size);

	for (ht = *h; ht; ht = ht->next) {
		if (KEY_EQ(key, ht->key)) {
			if (ht->next)
				ht->next->prev = ht->prev;
			if (ht->prev)
				ht->prev->next = ht->next;
			else
				*h = ht->next;
			table->len--;
			z_free(ht->key);
			if(value){
				*value=ht->value;
			}
			z_free((char *) ht);
			return ht;
		}
	}
	return 0;
}

void zhash_delete_node(ZHASH *table, ZHASH_NODE *node, char **value){
	if (node->next)
		node->next->prev = node->prev;
	if (node->prev)
		node->prev->next = node->next;
	table->len--;
	z_free(node->key);
	if(value){
		*value=node->value;
	}
	z_free((char *) node);
}

void zhash_free(ZHASH *table, void (*free_fn) (char *,char*), char *ptr){
	unsigned i = table->size;
	ZHASH_NODE *ht;
	ZHASH_NODE *next;
	ZHASH_NODE **h = table->data;

	while (i-- > 0) {
		for (ht = *h++; ht; ht = next) {
			next = ht->next;
			z_free(ht->key);
			if (free_fn)
				(*free_fn) (ht->value,ptr);
			z_free((char *) ht);
		}
	}
	z_free((char *) table->data);
	z_free((char *) table);
}

void zhash_walk(ZHASH *table, void (*walk_fn) (ZHASH_NODE *, char *), char *ptr) {
	unsigned i = table->size;
	ZHASH_NODE **h = table->data;
	ZHASH_NODE *ht;

	while (i-- > 0)
		for (ht = *h++; ht; ht = ht->next)
			(*walk_fn) (ht, ptr);
}

ZHASH_NODE **zhash_list(ZHASH *table){
	ZHASH_NODE **list;
	ZHASH_NODE *member;
	int     count = 0;
	int     i;

	list = (ZHASH_NODE **) z_malloc(sizeof(*list) * (table->len + 1));
	for (i = 0; i < table->size; i++)
		for (member = table->data[i]; member != 0; member = member->next)
			list[count++] = member;
	list[count] = 0;
	return (list);
}
