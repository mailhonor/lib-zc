/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn
 * 2016-01-15
 * ================================
 */

#include "zcc/zcc_cdb.h"
#include "zcc/zcc_intpack.h"

zcc_namespace_begin;

struct builder_node_t
{
    builder_node_t *next{0};
    std::string key;
    std::string val;
};

struct builder_table_t
{
    builder_node_t **hash_node_vector{0};
    int hash_node_size{0};
    int count{0};
    int klen{0};
};

static void builder_table_reset_hash_node_size(builder_table_t *table, int _hash_node_size)
{
    int klen = table->klen;
    builder_node_t **hv = (builder_node_t **)zcc::calloc(_hash_node_size, sizeof(builder_node_t *));
    for (int ohi = 0; ohi < table->hash_node_size; ohi++)
    {
        builder_node_t *n1, *n2;
        for (n1 = table->hash_node_vector[ohi]; n1; n1 = n2)
        {
            n2 = n1->next;
            n1->next = 0;
            const char *key = n1->key.c_str();
            int thash = hash_djb(key, klen) % _hash_node_size;
            builder_node_t *node = 0, *node_last = 0;
            for (node = hv[thash]; node; node_last = node, node = node->next)
            {
                int r = std::memcmp(key, node->key.c_str(), klen);
                if (!r)
                {
                    zcc_fatal("it is impossible");
                }
                if (r < 0)
                {
                    break;
                }
            }
            n1->next = 0;
            if (node_last)
            {
                node_last->next = n1;
                n1->next = node;
            }
            else
            {
                n1->next = node;
                hv[thash] = n1;
            }
        }
    }

    zcc::free(table->hash_node_vector);
    table->hash_node_vector = hv;
    table->hash_node_size = _hash_node_size;
}

static builder_table_t *builder_table_create(int klen)
{
    builder_table_t *table = (builder_table_t *)zcc::calloc(1, sizeof(builder_table_t));
    table->hash_node_size = 3;
    table->hash_node_vector = (builder_node_t **)zcc::calloc(table->hash_node_size, sizeof(builder_node_t *));
    table->count = 0;
    table->klen = klen;
    return table;
}

static void builder_table_free(builder_table_t *table)
{
    int i;
    for (i = 0; i < table->hash_node_size; i++)
    {
        builder_node_t *node = table->hash_node_vector[i], *next;
        for (; node; node = next)
        {
            next = node->next;
            delete node;
        }
    }
    zcc::free(table->hash_node_vector);
    zcc::free(table);
}

static void builder_table_update_token(builder_table_t *table, const void *key, int klen, const void *val, int vlen)
{
    int thash = (hash_djb(key, klen) % (unsigned int)table->hash_node_size);
    builder_node_t *node, *node_last = 0, *node_new;
    for (node = table->hash_node_vector[thash]; node; node_last = node, node = node->next)
    {
        int r = std::memcmp(key, node->key.c_str(), klen);
        if (!r)
        {
            node->val.clear();
            node->val.append((const char *)val, vlen);
            return;
        }
        if (r < 0)
        {
            break;
        }
    }
    table->count++;
    node_new = new builder_node_t();
    node_new->next = 0;
    node_new->key.append((const char *)key, klen);
    node_new->val.append((const char *)val, vlen);

    if (node_last)
    {
        node_last->next = node_new;
        node_new->next = node;
    }
    else
    {
        node_new->next = node;
        table->hash_node_vector[thash] = node_new;
    }
    if (table->count > table->hash_node_size)
    {
        builder_table_reset_hash_node_size(table, table->hash_node_size * 2 + 1);
    }
}

struct cdb_builder_engine_t
{
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

static int builder_stream_fseek(cdb_builder_engine_t *builder, int offset, int whence)
{
    int cursor = -1;
    if (whence == SEEK_SET)
    {
        cursor = offset;
    }
    else if (whence == SEEK_END)
    {
        cursor = builder->_size - offset;
    }
    else
    {
        zcc_fatal("unknown whence: %d", whence);
    }
    if (cursor > builder->_size)
    {
        cursor = builder->_size;
    }
    if (cursor < 0)
    {
        cursor = 0;
    }
    builder->_cursor = cursor;
    return 0;
}

static int builder_stream_fwrite(const void *ptr, int size, int nmemb, cdb_builder_engine_t *builder)
{
    if (builder->_error)
    {
        return -1;
    }
    int need = size * nmemb;
    while (1)
    {
        if (builder->_capability - builder->_cursor > need)
        {
            break;
        }
        auto old_len = builder->_capability;
        builder->_capability *= 2;
        char *new_data = (char *)zcc::malloc(builder->_capability + 10);
        std::memcpy(new_data, builder->_data, old_len);
        zcc::free(builder->_data);
        builder->_data = new_data;
    }
    std::memcpy(builder->_data + builder->_cursor, ptr, need);
    builder->_cursor += need;

    if (builder->_cursor > builder->_size)
    {
        builder->_size = builder->_cursor;
    }
    return nmemb;
}

static int builder_stream_ftell(cdb_builder_engine_t *builder)
{
    return builder->_cursor;
}

static int max_key_len = 4096;
static cdb_builder_engine_t *cdb_builder_create()
{
    cdb_builder_engine_t *builder = (cdb_builder_engine_t *)zcc::calloc(1, sizeof(cdb_builder_engine_t));
    builder->table_vector = (builder_table_t **)zcc::calloc((max_key_len + 1), sizeof(builder_table_t *));
    builder->max_klen = 0;
    builder->count = 0;
    builder->val_length_min = 0;
    builder->val_length_max = 0;
    /* myfile */
    builder->_capability = 1024;
    builder->_data = (char *)zcc::malloc(builder->_capability + 10);
    builder->_size = 0;
    builder->_cursor = 0;
    builder->_error = 0;
    builder->_over = 0;
    return builder;
}

static void cdb_builder_free(cdb_builder_engine_t *builder)
{
    for (int i = 0; i < max_key_len + 1; i++)
    {
        if (builder->table_vector[i])
        {
            builder_table_free(builder->table_vector[i]);
        }
    }
    zcc::free(builder->table_vector);
    zcc::free(builder->_data);
    zcc::free(builder);
}

static void cdb_builder_update(cdb_builder_engine_t *builder, const void *key, int klen, const void *val, int vlen)
{
    if (klen < 0)
    {
        klen = std::strlen((const char *)key);
    }
    if (klen > max_key_len)
    {
        zcc_fatal("cdb_builder_update, key'len(%d) > max_key_len(%d)", klen, max_key_len);
    }
    if (klen > builder->max_klen)
    {
        builder->max_klen = klen;
    }
    if (builder->table_vector[klen] == 0)
    {
        builder->table_vector[klen] = builder_table_create(klen);
    }
    builder->count -= builder->table_vector[klen]->count;
    if (vlen < 0)
    {
        vlen = std::strlen((const char *)val);
    }
    if (vlen != 0)
    {
        if (builder->val_length_min == 0)
        {
            builder->val_length_min = vlen;
        }
        if (builder->val_length_max == 0)
        {
            builder->val_length_max = vlen;
        }
    }
    if (vlen > builder->val_length_max)
    {
        builder->val_length_max = vlen;
    }
    if (vlen < builder->val_length_min)
    {
        builder->val_length_min = vlen;
    }
    builder_table_update_token(builder->table_vector[klen], key, klen, val, vlen);
    builder->count += builder->table_vector[klen]->count;
}

static int _cint_put(int size, char *buf)
{
    int ch, left = size, len = 0;
    do
    {
        ch = left & 0177;
        left >>= 7;
        if (!left)
        {
            ch |= 0200;
        }
        ((unsigned char *)buf)[len++] = ch;
    } while (left);
    return len;
}

static int cdb_builder_compile(cdb_builder_engine_t *builder)
{
    if (builder->_error)
    {
        return -1;
    }
    if (builder->_over)
    {
        return 1;
    }
    builder->_over = 1;

    unsigned char intbuf[64 + 1];
    int val_length;

    std::strcpy((char *)intbuf, "ZCDB");
    std::strcpy((char *)intbuf + 4, cdb_code_version);
    std::memset((char *)intbuf + 8, 0, 4);
    int_pack(builder->count, intbuf + 12);
    int_pack(builder->max_klen, intbuf + 16);
    if (builder->val_length_min == builder->val_length_max)
    {
        int_pack(builder->count, intbuf + 20);
        val_length = builder->val_length_max;
    }
    else
    {
        int_pack(536870912, intbuf + 20);
        val_length = 536870912;
    }
    int_pack(builder->count, intbuf + 4 + 4);

    if (builder_stream_fwrite(intbuf, 1, 24, builder) != 24)
    {
    }

    for (int tbi = 0; tbi <= builder->max_klen; tbi++)
    {
        if (builder_stream_fwrite("\0\0\0\0", 1, 4, builder) != 4)
        {
        }
    }

    for (int tbi = 0; tbi <= builder->max_klen; tbi++)
    {
        builder_stream_fseek(builder, 0, SEEK_END);
        if (builder_stream_ftell(builder) > 2147483648 - 1024 * 1024 * 100)
        {
            zcc_fatal("cdb only support size of 2G");
        }
        unsigned int offset1 = builder_stream_ftell(builder);
        builder_stream_fseek(builder, 24 + 4 * tbi, SEEK_SET);
        builder_table_t *ht = builder->table_vector[tbi];
        if (ht == 0)
        {
            builder_stream_fwrite("\0\0\0\0", 1, 4, builder);
            continue;
        }
        int_pack(offset1, intbuf);
        builder_stream_fwrite(intbuf, 1, 4, builder);
        builder_stream_fseek(builder, 0, SEEK_END);

        int_pack((unsigned int)ht->hash_node_size, intbuf);
        builder_stream_fwrite(intbuf, 1, 4, builder);
        offset1 = builder_stream_ftell(builder);
        for (int hi = 0; hi < ht->hash_node_size; hi++)
        {
            builder_stream_fwrite("\0\0\0\0", 1, 4, builder);
        }
        for (int hi = 0; hi < ht->hash_node_size; hi++)
        {
            builder_stream_fseek(builder, 0, SEEK_END);
            unsigned int offset2 = builder_stream_ftell(builder);
            builder_stream_fseek(builder, offset1 + 4 * hi, SEEK_SET);
            builder_node_t *node = ht->hash_node_vector[hi];
            if (node == 0)
            {
                builder_stream_fwrite("\0\0\0\0", 1, 4, builder);
                continue;
            }
            int_pack(offset2, intbuf);
            builder_stream_fwrite(intbuf, 1, 4, builder);

            builder_stream_fseek(builder, 0, SEEK_END);
            int ncount = 0;
            for (node = ht->hash_node_vector[hi]; node; node = node->next)
            {
                ncount++;
            }
            int_pack(ncount, intbuf);
            builder_stream_fwrite(intbuf, 1, 4, builder);
            if (val_length == 536870912)
            {
                unsigned int offset3 = builder_stream_ftell(builder);
                for (node = ht->hash_node_vector[hi]; node; node = node->next)
                {
                    if (tbi)
                    {
                        builder_stream_fwrite(node->key.c_str(), 1, tbi, builder);
                    }
                    builder_stream_fwrite("\0\0\0\0", 1, 4, builder);
                }
                int ni = 0;
                for (node = ht->hash_node_vector[hi]; node; node = node->next, ni++)
                {
                    int vlen = node->val.size();
                    if (vlen == 0)
                    {
                        continue;
                    }
                    builder_stream_fseek(builder, 0, SEEK_END);
                    unsigned int offset4 = builder_stream_ftell(builder);
                    int slen = _cint_put(vlen, (char *)intbuf);
                    builder_stream_fwrite(intbuf, 1, slen, builder);
                    builder_stream_fwrite(node->val.c_str(), 1, vlen, builder);
                    builder_stream_fseek(builder, (offset3 + (tbi + 4) * ni) + tbi, SEEK_SET);
                    int_pack(offset4, intbuf);
                    builder_stream_fwrite(intbuf, 1, 4, builder);
                }
            }
            else
            {
                for (node = ht->hash_node_vector[hi]; node; node = node->next)
                {
                    if (tbi)
                    {
                        builder_stream_fwrite(node->key.c_str(), 1, tbi, builder);
                    }
                    if (val_length)
                    {
                        builder_stream_fwrite(node->val.c_str(), 1, val_length, builder);
                    }
                }
            }
        }
    }

    builder_stream_fseek(builder, 0, SEEK_END);
    unsigned dblen = builder_stream_ftell(builder);
    builder_stream_fseek(builder, 8, SEEK_SET);
    int_pack(dblen, intbuf);
    builder_stream_fwrite(intbuf, 1, 4, builder);

    builder_stream_fseek(builder, 20, SEEK_SET);
    int_pack(val_length, intbuf);
    builder_stream_fwrite(intbuf, 1, 4, builder);

    if (builder->_error)
    {
        return -1;
    }

    return 1;
}

static int cdb_builder_build(cdb_builder_engine_t *builder, const char *dest_db_pathname)
{
    if (cdb_builder_compile(builder) < 1)
    {
        return -1;
    }
    if (zcc::file_put_contents(dest_db_pathname, builder->_data, builder->_size) < 1)
    {
        return -1;
    }
    return 1;
}

static const void *cdb_builder_get_compiled_data(cdb_builder_engine_t *builder)
{
    return builder->_data;
}

static int cdb_builder_get_compiled_len(cdb_builder_engine_t *builder)
{
    return builder->_size;
}

cdb_builder::cdb_builder()
{
    engine_ = cdb_builder_create();
}

cdb_builder::~cdb_builder()
{
    if (engine_)
    {
        cdb_builder_free(engine_);
        engine_ = nullptr;
    }
}

void cdb_builder::update(const void *key, int klen, const void *val, int vlen)
{
    cdb_builder_update(engine_, key, klen, val, vlen);
}

bool cdb_builder::compile()
{
    if (cdb_builder_compile(engine_) < 1)
    {
        return false;
    }
    return true;
}

const void *cdb_builder::get_compiled_data()
{
    return cdb_builder_get_compiled_data(engine_);
}

int cdb_builder::get_compiled_data_len()
{
    return cdb_builder_get_compiled_len(engine_);
}

int cdb_builder::dump(const char *dest_pathname)
{
    return cdb_builder_build(engine_, dest_pathname);
}

zcc_namespace_end;
