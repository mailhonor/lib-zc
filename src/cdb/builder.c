/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn
 * 2016-01-15
 * ================================
 */

#include "zc.h"

typedef struct builder_node_t builder_node_t;
struct builder_node_t {
    builder_node_t *next;
    zbuf_t key;
    zbuf_t val;
};

typedef struct builder_table_t builder_table_t;
struct builder_table_t {
    builder_node_t **hash_node_vector;
    int hash_node_size;
    int count;
    int klen;
};

static void builder_table_reset_hash_node_size(builder_table_t *table, int _hash_node_size)
{
    int klen = table->klen;
    builder_node_t **hv = (builder_node_t **)zcalloc(_hash_node_size, sizeof(builder_node_t *));
    for (int ohi = 0; ohi < table->hash_node_size; ohi++) {
        builder_node_t *n1, *n2;
        for (n1 = table->hash_node_vector[ohi]; n1; n1 = n2) {
            n2 = n1->next;
            n1->next = 0;
            char *key = zbuf_data(&(n1->key));
            int thash = zhash_djb(key, klen) %_hash_node_size;
            builder_node_t *node = 0, *node_last = 0;
            for (node = hv[thash]; node; node_last = node, node = node->next) {
                int r = memcmp(key, zbuf_data(&(node->key)), klen);
                if (!r) {
                    zfatal("it is impossible\n");
                }
                if (r < 0) {
                    break;
                }
            }
            n1->next = 0;
            if (node_last) {
                node_last->next = n1;
                n1->next = node;
            } else {
                n1->next = node;
                hv[thash] = n1;
            }
        }
    }

    zfree(table->hash_node_vector);
    table->hash_node_vector = hv;
    table->hash_node_size = _hash_node_size;
}


static builder_table_t * builder_table_create(int klen)
{
    builder_table_t *table = (builder_table_t *)zcalloc(1, sizeof(builder_table_t));
    table->hash_node_size = 3;
    table->hash_node_vector = (builder_node_t **)zcalloc(table->hash_node_size, sizeof(builder_node_t*));
    table->count = 0;
    table->klen = klen;
    return table;
}

static void builder_table_free(builder_table_t *table)
{
    int i;
    for (i=0;i<table->hash_node_size;i++) {
        builder_node_t *node = table->hash_node_vector[i], *next;
        for (;node;node=next) {
            next = node->next;
            zbuf_fini(&(node->key));
            zbuf_fini(&(node->val));
            zfree(node);
        }
    }
    zfree(table->hash_node_vector);
    zfree(table);
}

static void builder_table_update_token(builder_table_t *table, const void *key, int klen, const void *val, int vlen)
{
    int thash = (zhash_djb(key, klen) % (unsigned int )table->hash_node_size);
    builder_node_t *node, *node_last = 0, *node_new;
    for (node = table->hash_node_vector[thash]; node; node_last = node, node = node->next) {
        int r = memcmp(key, zbuf_data(&(node->key)), klen);
        if (!r) {
            zbuf_memcpy(&(node->val), val, vlen);
            return;
        }
        if (r < 0) {
            break;
        }
    }
    table->count ++;
    node_new = (builder_node_t *)zmalloc(sizeof(builder_node_t) + klen);
    node_new->next = 0;
    zbuf_init(&(node_new->key), klen);
    zbuf_memcpy(&(node_new->key), key, klen);
    zbuf_init(&(node_new->val), vlen);
    zbuf_memcpy(&(node_new->val), val, vlen);

    if (node_last) {
        node_last->next = node_new;
        node_new->next = node;
    } else {
        node_new->next = node;
        table->hash_node_vector[thash] = node_new;
    }
    if (table->count > table->hash_node_size) {
        builder_table_reset_hash_node_size(table, table->hash_node_size * 2 + 1);
    }
}

struct zcdb_builder_t {
    builder_table_t **table_vector;
    int max_klen;
    int count;
    int val_length_min;
    int val_length_max;
    /* myfile */
    char *_data;
    int _capability;
    int _size;
    int _cursor;
    char _error;
    char _over;
};

static int _zcdb_builder_fseek(zcdb_builder_t *builder, int offset, int whence)
{
    int cursor;
    if (whence == SEEK_SET) {
        cursor = offset;
    } else if (whence == SEEK_END) {
        cursor = builder->_size - offset;
    } else {
        zfatal("unknown whence: %d", whence);
    }
    if (cursor > builder->_size) {
        cursor = builder->_size;
    }
    if (cursor < 0) {
        cursor = 0;
    }
    builder->_cursor = cursor;
    return 0;
}

static int _zcdb_builder_fwrite(const void *ptr, int size, int nmemb, zcdb_builder_t *builder)
{
    if (builder->_error) {
        return -1;
    }
    int need = size * nmemb;
    while (1) {
        if (builder->_capability - builder->_cursor > need) {
            break;
        }
        builder->_capability *= 2;
        builder->_data = (char *)zrealloc(builder->_data, builder->_capability + 10);
    }
    memcpy(builder->_data + builder->_cursor, ptr, need);
    builder->_cursor += need;

    if (builder->_cursor > builder->_size) {
        builder->_size = builder->_cursor;
    }
    return nmemb;
}

static int _zcdb_builder_ftell(zcdb_builder_t *builder)
{
    return builder->_cursor;
}

int zvar_zcdb_max_key_len = 4096;
zcdb_builder_t *zcdb_builder_create()
{
    zcdb_builder_t *builder = (zcdb_builder_t *)zcalloc(1, sizeof(zcdb_builder_t));
    builder->table_vector = (builder_table_t **)zcalloc(zvar_zcdb_max_key_len+1, sizeof(builder_table_t *));
    builder->max_klen = 0;
    builder->count = 0;
    builder->val_length_min = 0;
    builder->val_length_max = 0;
    /* myfile */
    builder->_capability = 1024;
    builder->_data = (char *)zmalloc(1024+10);
    builder->_size = 0;
    builder->_cursor = 0;
    builder->_error = 0;
    builder->_over = 0;
    return builder;
}

void zcdb_builder_free(zcdb_builder_t *builder)
{
    for (int i = 0; i < zvar_zcdb_max_key_len+1; i++) {
        if (builder->table_vector[i]) {
            builder_table_free(builder->table_vector[i]);
        }
    }
    zfree(builder->table_vector);
    zfree(builder->_data);
    zfree(builder);
}

void zcdb_builder_update(zcdb_builder_t *builder, const void *key, int klen, const void *val, int vlen)
{
    if (klen < 0) {
        klen = strlen((const char *)key);
    }
    if (klen > zvar_zcdb_max_key_len) {
        zfatal("zcdb_builder_update, key'len(%d) > zvar_zcdb_max_key_len(%d)", klen, zvar_zcdb_max_key_len);
    }
    if (klen > builder->max_klen) {
        builder->max_klen = klen;
    }
    if (builder->table_vector[klen] == 0) {
        builder->table_vector[klen] = builder_table_create(klen);
    }
    builder->count -= builder->table_vector[klen]->count;
    if (vlen < 0) {
        vlen = strlen((const char *)val);
    }
    if (vlen != 0) {
        if (builder->val_length_min == 0) {
            builder->val_length_min = vlen;
        }
        if (builder->val_length_max == 0) {
            builder->val_length_max = vlen;
        }
    }
    if (vlen > builder->val_length_max) {
        builder->val_length_max = vlen;
    }
    if (vlen < builder->val_length_min) {
        builder->val_length_min = vlen;
    }
    builder_table_update_token(builder->table_vector[klen], key, klen, val, vlen);
    builder->count += builder->table_vector[klen]->count;
}

zbool_t zcdb_builder_compile(zcdb_builder_t *builder)
{
    if (builder->_error) {
        return -1;
    }
    if (builder->_over) {
        return 1;
    }
    builder->_over = 1;

    unsigned char intbuf[64+1];
    int val_length;

    strcpy((char *)intbuf, "ZCDB");
    strcpy((char *)intbuf+4, zvar_cdb_code_version);
    memset((char *)intbuf+8, 0, 4);
    zint_pack(builder->count, intbuf+12);
    zint_pack(builder->max_klen, intbuf+16);
    if (builder->val_length_min == builder->val_length_max) {
        zint_pack(builder->count, intbuf + 20);
        val_length = builder->val_length_max;
    } else {
        zint_pack(536870912, intbuf + 20);
        val_length = 536870912;
    }
    zint_pack(builder->count, intbuf + 4 + 4);

    if (_zcdb_builder_fwrite(intbuf, 1, 24, builder) != 24) {
    }

    for (int tbi = 0; tbi <= builder->max_klen; tbi++) {
        if (_zcdb_builder_fwrite("\0\0\0\0", 1, 4, builder) != 4) {
        }
    }

    for (int tbi = 0; tbi <= builder->max_klen; tbi++) {
        _zcdb_builder_fseek(builder, 0, SEEK_END);
        if (_zcdb_builder_ftell(builder) > 2147483648 - 1024 * 1024 * 100) {
            zfatal("cdb only support size of 2G");
        }
        unsigned int offset1 = _zcdb_builder_ftell(builder);
        _zcdb_builder_fseek(builder, 24 + 4*tbi, SEEK_SET);
        builder_table_t *ht = builder->table_vector[tbi];
        if (ht == 0) {
            _zcdb_builder_fwrite("\0\0\0\0", 1, 4, builder);
            continue;
        }
        zint_pack(offset1, intbuf);
        _zcdb_builder_fwrite(intbuf, 1, 4, builder);
        _zcdb_builder_fseek(builder, 0, SEEK_END);

        zint_pack((unsigned int )ht->hash_node_size, intbuf);
        _zcdb_builder_fwrite(intbuf, 1, 4, builder);
        offset1 = _zcdb_builder_ftell(builder);
        for (int hi = 0; hi < ht->hash_node_size; hi++) {
            _zcdb_builder_fwrite("\0\0\0\0", 1, 4, builder);
        }
        for (int hi = 0; hi < ht->hash_node_size; hi++) {
            _zcdb_builder_fseek(builder, 0, SEEK_END);
            unsigned int offset2 = _zcdb_builder_ftell(builder);
            _zcdb_builder_fseek(builder, offset1 + 4*hi, SEEK_SET);
            builder_node_t *node = ht->hash_node_vector[hi];
            if (node== 0) {
                _zcdb_builder_fwrite("\0\0\0\0", 1, 4, builder);
                continue;
            }
            zint_pack(offset2, intbuf);
            _zcdb_builder_fwrite(intbuf, 1, 4, builder);

            _zcdb_builder_fseek(builder, 0, SEEK_END);
            int ncount = 0;
            for (node = ht->hash_node_vector[hi]; node;node=node->next) {
                ncount++;
            }
            zint_pack(ncount, intbuf);
            _zcdb_builder_fwrite(intbuf, 1, 4, builder);
            if (val_length == 536870912) {
                unsigned int offset3 = _zcdb_builder_ftell(builder);
                for (node = ht->hash_node_vector[hi]; node;node=node->next) {
                    if (tbi) {
                        _zcdb_builder_fwrite(zbuf_data(&(node->key)), 1, tbi, builder);
                    }
                    _zcdb_builder_fwrite("\0\0\0\0", 1, 4, builder);
                }
                int ni = 0;
                for (node = ht->hash_node_vector[hi]; node;node=node->next, ni++) {
                    int vlen = zbuf_len(&(node->val));
                    if (vlen == 0) {
                        continue;
                    }
                    _zcdb_builder_fseek(builder, 0, SEEK_END);
                    unsigned int offset4 = _zcdb_builder_ftell(builder);
                    int slen = zcint_put(vlen, (char *)intbuf);
                    _zcdb_builder_fwrite(intbuf, 1, slen, builder);
                    _zcdb_builder_fwrite(zbuf_data(&(node->val)), 1, vlen, builder);
                    _zcdb_builder_fseek(builder, (offset3 + (tbi+4)*ni) + tbi, SEEK_SET);
                    zint_pack(offset4, intbuf);
                    _zcdb_builder_fwrite(intbuf, 1, 4, builder);
                }
            } else {
                for (node = ht->hash_node_vector[hi]; node;node=node->next) {
                    if (tbi) {
                        _zcdb_builder_fwrite(zbuf_data(&(node->key)), 1, tbi, builder);
                    }
                    if (val_length) {
                        _zcdb_builder_fwrite(zbuf_data(&(node->val)), 1, val_length, builder);
                    }
                }
            }
        }
    }

    _zcdb_builder_fseek(builder, 0, SEEK_END);
    unsigned dblen= _zcdb_builder_ftell(builder);
    _zcdb_builder_fseek(builder, 8, SEEK_SET);
    zint_pack(dblen, intbuf);
    _zcdb_builder_fwrite(intbuf, 1, 4, builder);

    _zcdb_builder_fseek(builder, 20, SEEK_SET);
    zint_pack(val_length, intbuf);
    _zcdb_builder_fwrite(intbuf, 1, 4, builder);

    if (builder->_error) {
        return -1;
    }

    return 1;
}

int zcdb_builder_build(zcdb_builder_t *builder, const char *dest_db_pathname)
{
    if (!zcdb_builder_compile(builder)) {
        return -1;
    }
    if (zfile_put_contents(dest_db_pathname, builder->_data, builder->_size) < 1) {
        return -1;
    }
    return 1;
}

const void *zcdb_builder_get_compiled_data(zcdb_builder_t *builder)
{
    return builder->_data;
}

int zcdb_builder_get_compiled_len(zcdb_builder_t *builder)
{
    return builder->_size;
}

