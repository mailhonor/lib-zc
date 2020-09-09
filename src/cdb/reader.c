/*
 * ================================
 * eli960@qq.com
 * www.mailhonor.com
 * 2016-01-15
 * ================================
 */

#include "zc.h"
/*
 * cdb 文件格式:
 * partA: 在文件开始
 * 4字节, 字符串, 文件类型,    ZCDB
 * 4字节, 字符串, 版本, 当前版本 0001
 * 4字节, int_pack, 安全码, 文件长度
 * 4字节, int pack, 成员数量
 * 4字节, int pack, 最大的KEY的长度, 记为 LENGHT_V
 * 4字节, int pack, 值的长度属性, 536870912:长度不定, 其他值:长度固定(记为VL),特别的0表示没有值
 * 
 * partB, LENGHT_V 个 4字节(偏移), 按key的年度顺排
 * 4字节 * LENGHT_V, int pack, 偏移, 指向 partC
 *
 * partC, 
 * 4字节, int pack, hash_vector_size != 0
 * 4字节 * hash_vector_size, int_pack, 指向 partD
 *
 * partD1, 值的长度(VL)固定
 * 4字节, int_pack, 节点个数, 记为 NODE_COUNT
 * (key的长度+VL) * NODE_COUNT
 * 
 * partD2, 值的长度不固定
 * 4字节, int_pack, 节点个数, 记为 NODE_COUNT
 * (key的长度 + 4字节偏移(指向值partE)) * NODE_COUNT,
 *
 * partE,
 * 字节数不定, size_data 序列化, 值的长度
 * 值
 *
 */

struct zcdb_t {
    zmmap_reader_t reader;
    int max_key_length;
    int val_length;
    int count;
};

zcdb_t *zcdb_open2(const char *cdb_pathname, zbuf_t *error_msg)
{
    if (error_msg) {
        zbuf_reset(error_msg);
    }
    zcdb_t *reader = (zcdb_t *)zcalloc(1, sizeof(zcdb_t));
    if (zmmap_reader_init(&(reader->reader), cdb_pathname) < 1) {
        zfree(reader);
        if (error_msg) {
            zbuf_printf_1024(error_msg, "can not open(%m)");
        }
        return 0;
    }
    char *data = reader->reader.data;
    int len = reader->reader.len;
    if (len < 12) {
        if (error_msg) {
            zbuf_printf_1024(error_msg, "data too short, data format error");
        }
        goto err;
    }
    if (strncmp(data, "ZCDB", 4)) {
        if (error_msg) {
            zbuf_printf_1024(error_msg, "not zc-cdb file, data format error");
        }
        goto err;
    }
    if (strncmp(data+4, zvar_cdb_code_version, 4)) {
        if (error_msg) {
            char buf[8];
            memcpy(buf, data+4, 4);
            buf[4] = 0;
            zbuf_printf_1024(error_msg, "version mismatch, code version:%s, data version:%s", zvar_cdb_code_version, buf);
        }
        goto err;
    }
    int file_len = zint_unpack(data+8);
    if (file_len > len) {
        if (error_msg) {
            zbuf_printf_1024(error_msg, "data short data format error");
        }
        goto err;
    }
    reader->count = zint_unpack(data + 12);
    reader->max_key_length = zint_unpack(data + 16);
    reader->val_length = zint_unpack(data + 20);
    return reader;
err:
    zmmap_reader_fini(&(reader->reader));
    zfree(reader);
    return 0;
}

zcdb_t *zcdb_open(const char *cdb_pathname)
{
    return zcdb_open2(cdb_pathname, 0);
}

int zcdb_find(zcdb_t *cdb, const void *key, int klen, void **val, int *vlen)
{
    char *data_begin = cdb->reader.data, *data;
    int offset, hash_vector_size, ncount;
    
    if (val) {
        *val = 0;
    }
    if (vlen) {
        *vlen = 0;
    }
    if (klen < 0) {
        klen = strlen((const char *)key);
    }
    if (klen > cdb->max_key_length) {
        return 0;
    }


    data = data_begin + 24 + 4*klen; /* partB */
    offset = zint_unpack(data);
    if (offset == 0) {
        return 0;
    }
    data = data_begin + offset; /* partC */
    hash_vector_size = zint_unpack(data);
    data += 4;

    offset = (klen?zhash_djb(key, klen):0)%((unsigned int)hash_vector_size);
    data = data + 4 * offset;
    offset = zint_unpack(data);
    if (offset == 0) {
        return 0;
    }
    data = data_begin + offset;
    /* data point to partD */

    ncount = zint_unpack(data);
    data += 4;
    if (ncount < 1) {
        return 0;
    }
    int val_length = cdb->val_length;
    if (val_length == 536870912) {
        val_length = 4;
    }
    int left = 0 , right=ncount-1, middle, cmp_r;
    char *key_p = 0;
    while(left <= right) {
        middle = (left+right)/2;
        key_p = data + middle*(klen + val_length);
        cmp_r = memcmp(key, key_p, klen);
        if (cmp_r == 0) {
            break;
        }
        key_p = 0;
        if (cmp_r < 0) {
            right = middle -1;
        } else {
            left = middle +1;
        }
    }
    if (key_p == 0) {
        return 0;
    }

    if (cdb->val_length == 536870912) {
        offset = zint_unpack(key_p + klen);
        if (offset == 0) {
            if (val) {
                *val = zblank_buffer;
            }
            if (vlen) {
                *vlen = 0;
            }
        } else {
            data = data_begin + offset;
            if (val || vlen) {
                int ch, size = 0, shift = 0;
                while (1) {
                    ch = *data++;
                    size |= ((ch & 0177) << shift);
                    if (ch & 0200) {
                        break;
                    }
                    shift += 7;
                }
                if (val) {
                    *val = data;
                }
                if (vlen) {
                    *vlen = size;
                }
            }

        }
    } else {
        if (val) {
            *val = key_p + klen;
        }
        if (vlen) {
            *vlen = cdb->val_length;
        }
    }

    return 1;
}

int zcdb_get_count(zcdb_t *cdb)
{
    return cdb->count;
}

void zcdb_close(zcdb_t *cdb)
{
    if (!cdb) {
        return;
    }
    zmmap_reader_fini(&(cdb->reader));
    zfree(cdb);
}

/* walker */
struct zcdb_walker_t {
    zcdb_t *cdb;
    int klen;
    char *hash_vector_data;
    int hash_vector_size;
    int hash_vector_i;
    char *ndata; /* n means node */
    int ncount;
    int ni;
};

zcdb_walker_t *zcdb_walker_create(zcdb_t *cdb)
{
    zcdb_walker_t *walker = zcalloc(1, sizeof(zcdb_walker_t));
    walker->cdb = cdb;
    walker->klen = -1;
    return walker;
}

static int zcdb_walker_get_next_ok(char *data_begin, char *data, int sn, int key_length, int val_length, void **key, int *klen, void **val, int *vlen)
{
    data += (key_length+((val_length==536870912)?4:val_length)) * sn;

    if (key) {
        *key = data;
    }
    if (klen) {
        *klen = key_length;
    }

    if (val_length == 536870912) {
        int offset = zint_unpack(data + key_length);
        if (offset == 0) {
            if (val) {
                *val = zblank_buffer;
            }
            if (vlen) {
                *vlen = 0;
            }
        } else {
            data = data_begin + offset;
            int ch, size = 0, shift = 0;
            while (1) {
                ch = *data++;
                size |= ((ch & 0177) << shift);
                if (ch & 0200) {
                    break;
                }
                shift += 7;
            }
            if (val) {
                *val = data;
            }
            if (vlen) {
                *vlen = size;
            }
        }
    } else {
        if (val) {
            *val = data + key_length;
        }
        if (vlen) {
            *vlen = val_length;
        }
    }
    return 1;
}

int zcdb_walker_walk(zcdb_walker_t *walker, void **key, int *klen, void **val, int *vlen)
{
    if (walker->klen == -2) {
        return 0;
    }

    zcdb_t *cdb = walker->cdb;
    char *data_begin = cdb->reader.data;

    if (walker->klen == -1) {
        walker->klen = 0;
        walker->hash_vector_i = -1;
        walker->ni = -1;
    }
    for (;walker->klen <= cdb->max_key_length; walker->klen++) {
        int offset = zint_unpack(data_begin + 24 + 4 * walker->klen);
        if (offset == 0) {
            continue;
        }
        if (walker->hash_vector_i == -1) {
            walker->hash_vector_data = data_begin + offset + 4;
            walker->hash_vector_size = zint_unpack(walker->hash_vector_data - 4);
            walker->hash_vector_i = 0;
            walker->ni = -1;
        }
        for (; walker->hash_vector_i < walker->hash_vector_size; walker->hash_vector_i++) {
            int offset = zint_unpack(walker->hash_vector_data + 4 * walker->hash_vector_i);
            if (offset == 0) {
                continue;
            }
            if (walker->ni == -1) {
                walker->ndata = data_begin + offset + 4;
                walker->ncount = zint_unpack(data_begin + offset);
                walker->ni = 0;
            }
            for (; walker->ni < walker->ncount;) {
                if (zcdb_walker_get_next_ok(data_begin, walker->ndata, walker->ni, walker->klen, cdb->val_length, key, klen, val, vlen) < 1) {
                    return -1;
                }
                walker->ni++;
                return 1;
            }
            walker->ni = -1;
        }
        walker->hash_vector_i = -1;
    }

    if (walker->klen > cdb->max_key_length) {
        walker->klen = -2;
        return 0;
    }
    return 0;
}

void zcdb_walker_reset(zcdb_walker_t *walker)
{
    zcdb_t *cdb = walker->cdb;
    memset(walker, 0, sizeof(zcdb_walker_t));
    walker->cdb = cdb;
    walker->klen = -1;
}

void zcdb_walker_free(zcdb_walker_t *walker)
{
    zfree(walker);
}
