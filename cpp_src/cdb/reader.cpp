/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn
 * 2016-01-15
 * ================================
 */

/*
 * cdb 文件格式:
 * partA: 在文件开始
 * 4字节, 字符串, 文件类型,       ZCDB
 * 4字节, 字符串, 版本, 当前版本, 0001
 * 4字节, int_pack, 安全码, 文件长度
 * 4字节, int pack, 成员数量
 * 4字节, int pack, 最大的KEY的长度, 记为 LENGHT_V
 * 4字节, int pack, 值的长度的属性, 536870912:长度不定, 其他值:长度固定(记为VL),特别的0表示没有值
 *
 * partB, LENGHT_V 个 4字节(偏移), 按key的长度顺排
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

#include "zcc/zcc_cdb.h"
#include "zcc/zcc_intpack.h"

zcc_namespace_begin;

const char *cdb_code_version = "0001";

bool cdb_reader::is_my_data(const void *data, int64_t size)
{
    const char *ps = (const char *)data;
    if (std::strncmp(ps, "ZCDB", 4))
    {
        return false;
    }
    if (std::strncmp(ps + 4, cdb_code_version, 4))
    {
        if (1)
        {
            char buf[5];
            std::memcpy(buf, ((const char *)data) + 4, 4);
            buf[4] = 0;
            zcc_warning("zcc cdb version mismatch, code version:%s, data version:%s", cdb_code_version, buf);
        }
        return false;
    }
    return true;
}

bool cdb_reader::is_my_file(const char *filename)
{
    bool r = false;
    constexpr int64_t min_size = 24 + 32;
    char buf[min_size];
    FILE *fp = nullptr;

    if (!(fp = fopen(filename, "rb")))
    {
        goto over;
    }
    if ((int64_t)fread(buf, 1, min_size, fp) != min_size)
    {
        goto over;
    }
    if (!is_my_data(buf, min_size))
    {
        goto over;
    }

    r = true;
over:
    if (fp)
    {
        fclose(fp);
    }
    return r;
}

cdb_reader *cdb_reader::create_from_file(const char *pathname)
{
    auto r = new cdb_reader();
    if (r->open_file(pathname))
    {
        return r;
    }
    delete r;
    return nullptr;
}

cdb_reader *cdb_reader::create_from_data(const void *data)
{
    auto r = new cdb_reader();
    r->open_data(data);
    return r;
}

cdb_reader::cdb_reader()
{
}

cdb_reader::~cdb_reader()
{
    close();
}

bool cdb_reader::open_data_do(const void *data)
{
    if (std::strncmp((const char *)data, "ZCDB", 4))
    {
        goto err;
    }
    if (std::strncmp(((const char *)data) + 4, cdb_code_version, 4))
    {
        if (1)
        {
            char buf[5];
            std::memcpy(buf, ((const char *)data) + 4, 4);
            buf[4] = 0;
            zcc_warning("zcc cdb version mismatch, code version:%s, data version:%s", cdb_code_version, buf);
        }
        goto err;
    }
    data_ = (char *)(void *)data;
    count_ = int_unpack(((const char *)data) + 12);
    max_key_length_ = int_unpack(((const char *)data) + 16);
    val_length_ = int_unpack(((const char *)data) + 20);
    return true;
err:
    return false;
}

bool cdb_reader::open_data(const void *data)
{
    if (used_flag_)
    {
        close();
    }
    bool r = open_data_do(data);
    if (r)
    {
        used_flag_ = true;
        file_flag_ = false;
    }
    return r;
}

bool cdb_reader::open_file(const char *pathname)
{
    if (file_reader_.open(pathname) < 1)
    {
        return false;
    }
    int len, file_len;
    data_ = file_reader_.data_;
    len = file_reader_.size_;
    if (len < 12)
    {
        goto err;
    }
    file_len = int_unpack(data_ + 8);
    if (file_len > len)
    {
        goto err;
    }
    if (!open_data_do(data_))
    {
        goto err;
    }
    used_flag_ = true;
    file_flag_ = true;
    return true;

err:
    close();
    return false;
}

void cdb_reader::close()
{
    file_reader_.close();
    used_flag_ = false;
    file_flag_ = false;
}

int cdb_reader::find(const void *key, int klen, void **val, int *vlen)
{
    char *data_begin = (char *)(void *)data_, *data;
    int offset, hash_vector_size, ncount;

    if (val)
    {
        *val = 0;
    }
    if (vlen)
    {
        *vlen = 0;
    }
    if (klen < 0)
    {
        klen = std::strlen((const char *)key);
    }
    if (klen > max_key_length_)
    {
        return 0;
    }

    data = data_begin + 24 + 4 * klen; /* partB */
    offset = int_unpack(data);
    if (offset == 0)
    {
        return 0;
    }
    data = data_begin + offset; /* partC */
    hash_vector_size = int_unpack(data);
    data += 4;

    offset = (klen ? hash_djb(key, klen) : 0) % ((unsigned int)hash_vector_size);
    data = data + 4 * offset;
    offset = int_unpack(data);
    if (offset == 0)
    {
        return 0;
    }
    data = data_begin + offset;
    /* data point to partD */

    ncount = int_unpack(data);
    data += 4;
    if (ncount < 1)
    {
        return 0;
    }
    int val_length = val_length_;
    if (val_length == 536870912)
    {
        val_length = 4;
    }
    int left = 0, right = ncount - 1, middle, cmp_r;
    char *key_p = 0;
    while (left <= right)
    {
        middle = (left + right) / 2;
        key_p = data + middle * (klen + val_length);
        cmp_r = memcmp(key, key_p, klen);
        if (cmp_r == 0)
        {
            break;
        }
        key_p = 0;
        if (cmp_r < 0)
        {
            right = middle - 1;
        }
        else
        {
            left = middle + 1;
        }
    }
    if (key_p == 0)
    {
        return 0;
    }

    if (val_length_ == 536870912)
    {
        offset = int_unpack(key_p + klen);
        if (offset == 0)
        {
            if (val)
            {
                *val = var_blank_buffer;
            }
            if (vlen)
            {
                *vlen = 0;
            }
        }
        else
        {
            data = data_begin + offset;
            if (val || vlen)
            {
                int ch, size = 0, shift = 0;
                while (1)
                {
                    ch = *data++;
                    size |= ((ch & 0177) << shift);
                    if (ch & 0200)
                    {
                        break;
                    }
                    shift += 7;
                }
                if (val)
                {
                    *val = data;
                }
                if (vlen)
                {
                    *vlen = size;
                }
            }
        }
    }
    else
    {
        if (val)
        {
            *val = key_p + klen;
        }
        if (vlen)
        {
            *vlen = val_length_;
        }
    }

    return 1;
}

int cdb_reader::get_count()
{
    return count_;
}

zcc_namespace_end;
