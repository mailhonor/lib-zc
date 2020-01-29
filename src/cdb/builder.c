/*
 * ================================
 * eli960@qq.com
 * www.mailhonor.com
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
                    zfatal("FATAL it is impossible\n");
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
    node_new = (builder_node_t *)malloc(sizeof(builder_node_t) + klen);
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
};

int zvar_zcdb_max_key_len = 4096;
zcdb_builder_t *zcdb_builder_create()
{
    zcdb_builder_t *builder = (zcdb_builder_t *)zcalloc(1, sizeof(zcdb_builder_t));
    builder->table_vector = (builder_table_t **)zcalloc(zvar_zcdb_max_key_len+1, sizeof(builder_table_t *));
    builder->max_klen = 0;
    builder->count = 0;
    builder->val_length_min = 0;
    builder->val_length_max = 0;
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
    zfree(builder);
}

void zcdb_builder_update(zcdb_builder_t *builder, const void *key, int klen, const void *val, int vlen)
{
    if (klen < 0) {
        klen = strlen((const char *)key);
    }
    if (klen > zvar_zcdb_max_key_len) {
        zfatal("FATAL zcdb_builder_update, key'len(%d) > zvar_zcdb_max_key_len(%d)", klen, zvar_zcdb_max_key_len);
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

int zcdb_builder_build(zcdb_builder_t *builder, const char *dest_db_pathname)
{
    unsigned char intbuf[64+1];
    FILE *fp = 0;
    int val_length;

    fp = fopen(dest_db_pathname, "w");
    if (!fp) {
        return 0;
    }

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

    if (fwrite(intbuf, 1, 24, fp) != 24) {
    }

    for (int tbi = 0; tbi <= builder->max_klen; tbi++) {
        if (fwrite("\0\0\0\0", 1, 4, fp) != 4) {
        }
    }

    for (int tbi = 0; tbi <= builder->max_klen; tbi++) {
        fseek(fp, 0, SEEK_END);
        unsigned int offset1 = ftell(fp);
        fseek(fp, 24 + 4*tbi, SEEK_SET);
        builder_table_t *ht = builder->table_vector[tbi];
        if (ht == 0) {
            fwrite("\0\0\0\0", 1, 4, fp);
            continue;
        }
        zint_pack(offset1, intbuf);
        fwrite(intbuf, 1, 4, fp);
        fseek(fp, 0, SEEK_END);

        zint_pack((unsigned int )ht->hash_node_size, intbuf);
        fwrite(intbuf, 1, 4, fp);
        offset1 = ftell(fp);
        for (int hi = 0; hi < ht->hash_node_size; hi++) {
            fwrite("\0\0\0\0", 1, 4, fp);
        }
        for (int hi = 0; hi < ht->hash_node_size; hi++) {
            fseek(fp, 0, SEEK_END);
            unsigned int offset2 = ftell(fp);
            fseek(fp, offset1 + 4*hi, SEEK_SET);
            builder_node_t *node = ht->hash_node_vector[hi];
            if (node== 0) {
                fwrite("\0\0\0\0", 1, 4, fp);
                continue;
            }
            zint_pack(offset2, intbuf);
            fwrite(intbuf, 1, 4, fp);

            fseek(fp, 0, SEEK_END);
            int ncount = 0;
            for (node = ht->hash_node_vector[hi]; node;node=node->next) {
                ncount++;
            }
            zint_pack(ncount, intbuf);
            fwrite(intbuf, 1, 4, fp);
            if (val_length == 536870912) {
                unsigned int offset3 = ftell(fp);
                for (node = ht->hash_node_vector[hi]; node;node=node->next) {
                    if (tbi) {
                        fwrite(zbuf_data(&(node->key)), 1, tbi, fp);
                    }
                    fwrite("\0\0\0\0", 1, 4, fp);
                }
                int ni = 0;
                for (node = ht->hash_node_vector[hi]; node;node=node->next, ni++) {
                    int vlen = zbuf_len(&(node->val));
                    if (vlen == 0) {
                        continue;
                    }
                    fseek(fp, 0, SEEK_END);
                    unsigned int offset4 = ftell(fp);
                    int slen = zcint_put(vlen, (char *)intbuf);
                    fwrite(intbuf, 1, slen, fp);
                    fwrite(zbuf_data(&(node->val)), 1, vlen, fp);
                    fseek(fp, (offset3 + (tbi+4)*ni) + tbi, SEEK_SET);
                    zint_pack(offset4, intbuf);
                    fwrite(intbuf, 1, 4, fp);
                }
            } else {
                for (node = ht->hash_node_vector[hi]; node;node=node->next) {
                    if (tbi) {
                        fwrite(zbuf_data(&(node->key)), 1, tbi, fp);
                    }
                    if (val_length) {
                        fwrite(zbuf_data(&(node->val)), 1, val_length, fp);
                    }
                }
            }
        }
    }

    fseek(fp, 0, SEEK_END);
    unsigned dblen= ftell(fp);
    fseek(fp, 8, SEEK_SET);
    zint_pack(dblen, intbuf);
    fwrite(intbuf, 1, 4, fp);

    fseek(fp, 20, SEEK_SET);
    zint_pack(val_length, intbuf);
    fwrite(intbuf, 1, 4, fp);
    fflush(fp);

    if (ferror(fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);

    return 1;
}
