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

cdb_walker::cdb_walker()
{
    reset();
}

cdb_walker::cdb_walker(cdb_reader &reader)
{
    reset();
    set_reader(reader);
}

cdb_walker::~cdb_walker()
{
    close();
}

void cdb_walker::set_reader(cdb_reader &reader)
{
    reader_ = &reader;
    klen_ = -1;
}

void cdb_walker::close()
{
    if (!reader_)
    {
        return;
    }
    reset();
    reader_ = 0;
}

void cdb_walker::reset()
{
    klen_ = -1;
    hash_vector_data_ = 0;
    hash_vector_size_ = 0;
    hash_vector_i_ = 0;
    ndata_ = 0;
    ncount_ = 0;
    ni_ = 0;
}

static int get_next_ok(char *data_begin, char *data, int sn, int key_length, int val_length, void **key, int *klen, void **val, int *vlen)
{
    data += (key_length + ((val_length == 536870912) ? 4 : val_length)) * sn;

    if (key)
    {
        *key = data;
    }
    if (klen)
    {
        *klen = key_length;
    }

    if (val_length == 536870912)
    {
        int offset = int_unpack(data + key_length);
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
    else
    {
        if (val)
        {
            *val = data + key_length;
        }
        if (vlen)
        {
            *vlen = val_length;
        }
    }
    return 1;
}

int cdb_walker::walk(void **key, int *klen, void **val, int *vlen)
{
    if (klen_ == -2)
    {
        return 0;
    }
    cdb_reader *cdb = reader_;
    if (!cdb)
    {
        return false;
    }
    char *data_begin = (char *)(void *)(cdb->data_);

    if (klen_ == -1)
    {
        klen_ = 0;
        hash_vector_i_ = -1;
        ni_ = -1;
    }
    for (; klen_ <= cdb->max_key_length_; klen_++)
    {
        int offset = int_unpack(data_begin + 24 + 4 * klen_);
        if (offset == 0)
        {
            continue;
        }
        if (hash_vector_i_ == -1)
        {
            hash_vector_data_ = data_begin + offset + 4;
            hash_vector_size_ = int_unpack(hash_vector_data_ - 4);
            hash_vector_i_ = 0;
            ni_ = -1;
        }
        for (; hash_vector_i_ < hash_vector_size_; hash_vector_i_++)
        {
            int offset = int_unpack(hash_vector_data_ + 4 * hash_vector_i_);
            if (offset == 0)
            {
                continue;
            }
            if (ni_ == -1)
            {
                ndata_ = data_begin + offset + 4;
                ncount_ = int_unpack(data_begin + offset);
                ni_ = 0;
            }
            for (; ni_ < ncount_;)
            {
                if (get_next_ok(data_begin, ndata_, ni_, klen_, cdb->val_length_, key, klen, val, vlen) < 1)
                {
                    return -1;
                }
                ni_++;
                return 1;
            }
            ni_ = -1;
        }
        hash_vector_i_ = -1;
    }

    if (klen_ > cdb->max_key_length_)
    {
        klen_ = -2;
        return 0;
    }
    return 0;
}

zcc_namespace_end;
