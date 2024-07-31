/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2020-07-27
 * ================================
 */

#include <set>
#include "zcc/zcc_search.h"

#define _MAX_LINE_SIZE 4096

#define _CODE_ID "ZMSH"
#define _CODE_VERSION "\x00\x01\x00\x01"

#define zcc_msearch_debug(...)  \
    if (var_msearch_debug_mode) \
    zcc_info(__VA_ARGS__)

zcc_namespace_begin;

bool var_msearch_debug_mode = false;

struct _short_t
{
    short int i;
};
struct _int_t
{
    int i;
};

struct zcc_msearch_token_t
{
    short int len;
    /* void *data */
};

struct zcc_msearch_builder_t
{
    char a_data[256];
    std::set<std::string> *ab_data_tmp;
    std::set<std::string> *abc_data_tmp;
    unsigned char tmpkey[_MAX_LINE_SIZE + 1];
    unsigned char *mem_data;
    int mem_len;
    int mem_capability;
    char add_over;
};

struct zcc_msearch_reader_t
{
    char code_id[4];
    char code_version[4];
    int data_len;
    /* 01: 匹配成功 */
    /* 02: 双字节,第一个字节匹配成功, 04: 双字节, 第二个字节匹配成功 */
    /* 08: 三字字节,第一个字节匹配成功, 10: 二, 20: 三 */
    char a_data[256];

    /* offset ==> (_short_t + n * AB) */
    int ab_data_offset;
    int ab_size;

    /* */
    /* offset ==> (_int_t + n * (zcc_msearch_token_t *)) */
    int abc_data_offset;
    int abc_size;
};

static zcc_msearch_builder_t *init_builder_engine()
{
    zcc_msearch_builder_t *e = (zcc_msearch_builder_t *)calloc(1, sizeof(zcc_msearch_builder_t));
    e->ab_data_tmp = new std::set<std::string>();
    e->abc_data_tmp = new std::set<std::string>();
    return e;
}

msearch_builder::msearch_builder()
{
    builder_engine_ = nullptr;
}

msearch_builder::~msearch_builder()
{
    if (builder_engine_)
    {
        if (builder_engine_->ab_data_tmp)
        {
            delete builder_engine_->ab_data_tmp;
        }
        if (builder_engine_->abc_data_tmp)
        {
            delete builder_engine_->abc_data_tmp;
        }
        free(builder_engine_->mem_data);
        free(builder_engine_);
    }
}

void msearch_builder::add_token(const void *word, int len)
{
    if (!builder_engine_)
    {
        builder_engine_ = init_builder_engine();
    }
    if (builder_engine_->add_over)
    {
        return;
    }
    if (len < 0)
    {
        len = (int)std::strlen((const char *)word);
    }
    if (len < 1)
    {
        return;
    }
    if (len > _MAX_LINE_SIZE)
    {
        return;
    }
    unsigned char *key = builder_engine_->tmpkey;
    unsigned char *data = (unsigned char *)word;
    if (len == 1)
    {
        builder_engine_->a_data[data[0]] |= 0X01;
        return;
    }
    if (len == 2)
    {
        builder_engine_->a_data[data[0]] |= 0X02;
        builder_engine_->a_data[data[1]] |= 0X04;
        key[0] = data[0];
        key[1] = data[1];
        key[2] = 0;
        builder_engine_->ab_data_tmp->insert(std::string((const char *)word, len));
        return;
    }
    builder_engine_->a_data[data[0]] |= 0X08;
    builder_engine_->a_data[data[1]] |= 0X10;
    builder_engine_->a_data[data[2]] |= 0X20;
    std::memcpy(key, data, (size_t)len);
    key[len] = 0;
    builder_engine_->abc_data_tmp->insert(std::string((char *)key));
}

static int _mem_malloc(zcc_msearch_builder_t *m, int len)
{
    int changed = 0;
    while (m->mem_capability - m->mem_len < len)
    {
        m->mem_capability = 2 * m->mem_capability + 1;
        changed = 1;
    }
    if (changed)
    {
        unsigned char *tmp_data = (unsigned char *)malloc(m->mem_capability + 1);
        std::memcpy(tmp_data, m->mem_data, m->mem_len);
        free(m->mem_data);
        m->mem_data = tmp_data;
    }
    std::memset(m->mem_data + m->mem_len, 0, (size_t)len);
    int r = m->mem_len;
    m->mem_len += len;
    return r;
}

static void zcc_msearch_builder_add_over_1(zcc_msearch_builder_t *builder_engine_)
{
    if (builder_engine_ == 0)
    {
        zcc_fatal("zcc_msearch_builder_add_over already be excuted");
    }
    if (((zcc_msearch_builder_t *)-10 < builder_engine_) && (builder_engine_ < 0))
    {
        zcc_fatal("zcc_msearch_builder_add_over should not be excuted");
    }

    builder_engine_->mem_capability = 1024 * 1024;
    builder_engine_->mem_data = (unsigned char *)malloc(builder_engine_->mem_capability + 1);

    unsigned char **data = &(builder_engine_->mem_data);
    zcc_msearch_reader_t **engine = (zcc_msearch_reader_t **)data;
    _mem_malloc(builder_engine_, sizeof(zcc_msearch_reader_t));
    std::memcpy((*engine)->a_data, builder_engine_->a_data, 256);
}

static void zcc_msearch_builder_add_over_2(zcc_msearch_builder_t *builder_engine_)
{
    unsigned char **data = &(builder_engine_->mem_data);
    zcc_msearch_reader_t **engine = (zcc_msearch_reader_t **)data;

    int size = builder_engine_->ab_data_tmp->size();
    if (!size)
    {
        return;
    }
    size = 2 * size + 1;
    (*engine)->ab_size = size;
    (*engine)->ab_data_offset = _mem_malloc(builder_engine_, sizeof(int) * size);
    for (auto it = builder_engine_->ab_data_tmp->begin(); it != builder_engine_->ab_data_tmp->end(); it++)
    {
        const std::string &word = *it;
        int hv = (unsigned char)word[0];
        hv = (hv << 8) + (unsigned char)word[1];
        int *ab_data_ptr = (int *)((*data) + (*engine)->ab_data_offset);
        ab_data_ptr[hv % size]++;
    }
    for (int i = 0; i < size; i++)
    {
        int *ab_data_ptr = (int *)((*data) + (*engine)->ab_data_offset);
        if (ab_data_ptr[i])
        {
            ab_data_ptr[i] = _mem_malloc(builder_engine_, sizeof(_short_t) + 2 * ab_data_ptr[i]);
            _short_t *_s = (_short_t *)((*data) + ab_data_ptr[i]);
            _s->i = 0;
        }
    }

    for (auto it = builder_engine_->ab_data_tmp->begin(); it != builder_engine_->ab_data_tmp->end(); it++)
    {
        const std::string &word = *it;
        int hv = (unsigned char)word[0];
        hv = (hv << 8) + (unsigned char)word[1];
        int *ab_data_ptr = (int *)((*data) + (*engine)->ab_data_offset);
        _short_t *_s = (_short_t *)((*data) + ab_data_ptr[hv % size]);
        unsigned char *p12 = ((unsigned char *)_s) + sizeof(_short_t) + 2 * _s->i;
        p12[0] = (unsigned char)word[0];
        p12[1] = (unsigned char)word[1];
        _s->i++;
    }
}

static void zcc_msearch_builder_add_over_3_clear(zcc_msearch_builder_t *builder_engine_)
{
    int size = builder_engine_->abc_data_tmp->size();
    if (!size)
    {
        return;
    }
    std::set<std::string> del_keys_map;
    const char *last = 0;
    int llen;
    for (auto it = builder_engine_->abc_data_tmp->begin(); it != builder_engine_->abc_data_tmp->end(); it++)
    {
        const std::string &word = *it;
        if (last == 0)
        {
            last = word.c_str();
            llen = strlen(last);
        }
        else
        {
            if (!strncmp(last, word.c_str(), llen))
            {
                del_keys_map.insert(word);
            }
            else
            {
                last = word.c_str();
                llen = std::strlen(last);
            }
        }
    }

    for (auto it = del_keys_map.begin(); it != del_keys_map.end(); it++)
    {
        const std::string &word = *it;
        builder_engine_->abc_data_tmp->erase(word);
    }
}

static void zcc_msearch_builder_add_over_3(zcc_msearch_builder_t *builder_engine_)
{
    int tmpoffset;
    unsigned char **data = &(builder_engine_->mem_data);
    zcc_msearch_reader_t **engine = (zcc_msearch_reader_t **)data;

    int size = builder_engine_->abc_data_tmp->size();
    if (!size)
    {
        return;
    }
    size = 2 * size + 1;
    (*engine)->abc_size = size;
    tmpoffset = _mem_malloc(builder_engine_, sizeof(int) * size);
    (*engine)->abc_data_offset = tmpoffset;
    for (auto it = builder_engine_->abc_data_tmp->begin(); it != builder_engine_->abc_data_tmp->end(); it++)
    {
        const std::string &word = *it;
        int hv = (unsigned char)word[0];
        hv = (hv << 8) + (unsigned char)word[1];
        hv = (hv << 8) + (unsigned char)word[2];
        int *abc_data_ptr = (int *)((*data) + (*engine)->abc_data_offset);
        abc_data_ptr[hv % size]++;
    }
    for (int i = 0; i < size; i++)
    {
        int *abc_data_ptr = (int *)((*data) + (*engine)->abc_data_offset);
        if (abc_data_ptr[i])
        {
            tmpoffset = _mem_malloc(builder_engine_, sizeof(_int_t) + sizeof(int) * abc_data_ptr[i]);
            abc_data_ptr[i] = tmpoffset;
            _int_t *_i = (_int_t *)((*data) + abc_data_ptr[i]);
            _i->i = 0;
        }
    }
    for (auto it = builder_engine_->abc_data_tmp->begin(); it != builder_engine_->abc_data_tmp->end(); it++)
    {
        const std::string &word = *it;
        int hv = (unsigned char)word[0];
        hv = (hv << 8) + (unsigned char)word[1];
        hv = (hv << 8) + (unsigned char)word[2];
        int len = word.size();
        int offset = _mem_malloc(builder_engine_, sizeof(zcc_msearch_token_t) + len);
        int *abc_data_ptr = (int *)((*data) + (*engine)->abc_data_offset);
        _int_t *_i = (_int_t *)((*data) + abc_data_ptr[hv % size]);
        *((int *)(((char *)_i) + sizeof(_int_t) + sizeof(int) * (_i->i))) = offset;
        zcc_msearch_token_t *token = (zcc_msearch_token_t *)((*data) + offset);
        token->len = len;
        std::memcpy(((char *)token) + sizeof(zcc_msearch_token_t), word.c_str(), len);
        _i = (_int_t *)((*data) + abc_data_ptr[hv % size]);
        _i->i++;
    }
}

static void zcc_msearch_builder_add_over(zcc_msearch_builder_t *builder_engine_)
{
    if (!builder_engine_)
    {
        builder_engine_ = init_builder_engine();
    }
    builder_engine_->add_over = 1;
    zcc_msearch_reader_t *reader_engine_ = (zcc_msearch_reader_t *)(builder_engine_->mem_data);
    reader_engine_->data_len = builder_engine_->mem_len;
    std::memcpy(reader_engine_->code_id, _CODE_ID, 4);
    std::memcpy(reader_engine_->code_version, _CODE_VERSION, 4);
}

void msearch_builder::add_over()
{
    if (!builder_engine_)
    {
        builder_engine_ = init_builder_engine();
    }
    if (builder_engine_->add_over)
    {
        return;
    }
    zcc_msearch_builder_add_over_1(builder_engine_);
    zcc_msearch_builder_add_over_2(builder_engine_);
    zcc_msearch_builder_add_over_3_clear(builder_engine_);
    zcc_msearch_builder_add_over_3(builder_engine_);
    zcc_msearch_builder_add_over(builder_engine_);
}

const void *msearch_builder::get_compiled_data()
{
    if (!builder_engine_)
    {
        builder_engine_ = init_builder_engine();
    }
    if (!builder_engine_->add_over)
    {
        return var_blank_buffer;
    }
    return builder_engine_->mem_data;
}

int64_t msearch_builder::get_compiled_size()
{
    if (!builder_engine_)
    {
        builder_engine_ = init_builder_engine();
    }
    return builder_engine_->mem_len;
}

int msearch_builder::add_token_from_file(const char *pathname)
{
    if (!builder_engine_)
    {
        builder_engine_ = init_builder_engine();
    }
    char buf[4096 + 1];
    FILE *fp = fopen(pathname, "rb");
    if (!fp)
    {
        return -1;
    }
    while (fgets(buf, 4096, fp))
    {
        char *ps = buf;
        ps = trim(ps);
        if (*ps == 0)
        {
            continue;
        }
        if (!std::strncmp(ps, "###", 3))
        {
            continue;
        }
        add_token(ps, -1);
    }
    fclose(fp);
    return 1;
}

bool msearch_reader::is_my_data(const void *data, int64_t size)
{
    int64_t min_size = sizeof(zcc_msearch_reader_t);
    if (!data)
    {
        return false;
    }
    if (size < min_size)
    {
        return false;
    }
    zcc_msearch_reader_t *reader = (zcc_msearch_reader_t *)(data);
    if (std::memcmp(reader->code_id, _CODE_ID, 4))
    {
        return false;
    }

    if (std::memcmp(reader->code_version, _CODE_VERSION, 4))
    {
        return false;
    }
    return true;
}

bool msearch_reader::is_my_file(const char *filename)
{
    bool r = false;
    constexpr int64_t min_size = sizeof(zcc_msearch_reader_t) + 32;
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

msearch_reader *msearch_reader::create_from_data(const void *data)
{
    msearch_reader *n = new msearch_reader();
    n->load_from_data(data);
    return n;
}

msearch_reader *msearch_reader::create_from_file(const char *filename)
{
    msearch_reader *n = new msearch_reader();
    if (n->load_from_file(filename) < 1)
    {
        delete n;
        return nullptr;
    }
    return n;
}

msearch_reader::msearch_reader()
{
    reader_engine_ = nullptr;
}

msearch_reader::~msearch_reader()
{
    reader_engine_ = nullptr;
}

void msearch_reader::load_from_data(const void *data)
{
    reader_engine_ = (zcc_msearch_reader_t *)(void *)data;
    reader_type_ = 1;
}

int msearch_reader::load_from_file(const char *filename)
{
    int ret = reader_.open(filename);
    if (ret < 1)
    {
        return ret;
    }
    ret = -1;
    do
    {
        if (reader_.len_ < (int64_t)sizeof(zcc_msearch_reader_t))
        {
            break;
        }
        reader_engine_ = (zcc_msearch_reader_t *)(reader_.data_);
        if (reader_.len_ < reader_engine_->data_len)
        {
            break;
        }
        if (std::memcmp(reader_.data_, _CODE_ID, 4))
        {
            break;
        }

        if (std::memcmp((char *)(reader_.data_) + 4, _CODE_VERSION, 4))
        {
            break;
        }
        ret = 1;
    } while (0);
    if (ret < 1)
    {
        return ret;
    }
    reader_type_ = 2;
    return 1;
}

int msearch_reader::match(const char *str, int len, const char **matched_ptr, int *matched_len)
{
    if (len < 0)
    {
        len = std::strlen(str);
    }
    if (len < 1)
    {
        return 0;
    }

    zcc_msearch_reader_t *engine = reader_engine_;
    if (!engine)
    {
        return -1;
    }

    unsigned char *ustr = (unsigned char *)str;
    unsigned char *data = (unsigned char *)engine;
    unsigned char *a_data = (unsigned char *)(engine->a_data);
    int *ab_data = (int *)((char *)engine + engine->ab_data_offset);
    int ab_size = engine->ab_size;
    int *abc_data = (int *)((char *)engine + engine->abc_data_offset);
    int abc_size = engine->abc_size;
    int plen, firstch, hv, off, sti, edi, mdi, v;
    unsigned char *ps, *the_data;

    for (int i = 0; i < len; i++)
    {
        ps = ustr + i;
        plen = len - i;
        if (plen < 1)
        {
            return 0;
        }
        if (matched_ptr)
        {
            *matched_ptr = str + i;
        }
        firstch = a_data[ps[0]];
        if (firstch & 0X01)
        {
            if (matched_len)
            {
                *matched_len = 1;
            }
            return 1;
        }
        while ((plen > 1) && (firstch & 0X02) && (a_data[ps[1]] & 0X04))
        {
            hv = ps[0];
            hv = (hv << 8) + ps[1];
            off = ab_data[hv % ab_size];
            if (!off)
            {
                break;
            }
            sti = 0;
            edi = ((_short_t *)(data + off))->i;
            the_data = data + off + sizeof(_short_t);
            while (sti < edi)
            {
                mdi = (sti + edi) / 2;
                v = the_data[2 * mdi];
                v = (v << 8) + the_data[2 * mdi + 1];
                if (hv == v)
                {
                    if (matched_len)
                    {
                        *matched_len = 2;
                    }
                    return 2;
                }
                if (hv < v)
                {
                    edi--;
                    continue;
                }
                if (v < hv)
                {
                    sti++;
                    continue;
                }
            }
            break;
        }
        if ((plen > 2) && (firstch & 0X08) && (a_data[ps[1]] & 0X10) && (a_data[ps[2]] & 0x20))
        {
        }
        else
        {
            continue;
        }

        hv = ps[0];
        hv = (hv << 8) + ps[1];
        hv = (hv << 8) + ps[2];
        off = abc_data[hv % abc_size];
        if (!off)
        {
            continue;
        }
        sti = 0;
        edi = ((_int_t *)(data + off))->i;
        int *the_int_data = (int *)(data + off + sizeof(_int_t));
        while (sti < edi)
        {
            mdi = (sti + edi) / 2;
            zcc_msearch_token_t *token = (zcc_msearch_token_t *)(data + the_int_data[mdi]);
            int tlen = token->len;
            const void *tdata = (const char *)token + sizeof(zcc_msearch_token_t);
            if (plen < tlen)
            {
                int r = memcmp(ps, tdata, plen);
                if (r <= 0)
                {
                    edi = mdi;
                    continue;
                }
                else
                {
                    sti = mdi + 1;
                    continue;
                }
            }
            else
            {
                int r = memcmp(ps, tdata, tlen);
                if (r == 0)
                {
                    if (matched_len)
                    {
                        *matched_len = tlen;
                    }
                    return 1;
                }
                else if (r < 0)
                {
                    edi = mdi;
                    continue;
                }
                else
                {
                    sti = mdi + 1;
                    continue;
                }
            }
        }
    }
    return 0;
}

void msearch_reader::add_over()
{
    msearch_builder::add_over();
    load_from_data(get_compiled_data());
}

struct zcc_msearch_walker_t
{
    unsigned char abc[256];
    int stage;
    int hv;
    int idx;
};

msearch_walker::msearch_walker(msearch_reader &reader) : reader_(reader)
{
    walker_engine_ = (zcc_msearch_walker_t *)calloc(1, sizeof(zcc_msearch_walker_t));
    reset();
}

msearch_walker::~msearch_walker()
{
    free(walker_engine_);
}

void msearch_walker::reset()
{
    walker_engine_->stage = 0;
    walker_engine_->hv = 0;
    walker_engine_->idx = 0;
    for (int i = 0; i < 256; i++)
    {
        walker_engine_->abc[i] = i;
    }
}

int msearch_walker::walk(const char **token, int *tlen)
{
    zcc_msearch_reader_t *engine = reader_.reader_engine_;
    if (!engine)
    {
        return -1;
    }
    auto walker = walker_engine_;

    unsigned char *data = (unsigned char *)engine;
    unsigned char *a_data = (unsigned char *)(engine->a_data);
    int *ab_data = (int *)((char *)engine + engine->ab_data_offset);
    int ab_size = engine->ab_size;
    int *abc_data = (int *)((char *)engine + engine->abc_data_offset);
    int abc_size = engine->abc_size;
    int hv, off, sti, edi;
    unsigned char *the_data;

    if (walker->stage == 0)
    {
        for (hv = walker->hv; hv < 256; hv++)
        {
            if (a_data[hv] & 0X01)
            {
                *token = (char *)(&(walker->abc[hv]));
                if (tlen)
                {
                    *tlen = 1;
                }
                walker->hv = hv + 1;
                return 1;
            }
        }
        walker->stage = 1;
        walker->hv = 0;
        walker->idx = 0;
    }

    if (walker->stage == 1)
    {
        for (hv = walker->hv; hv < ab_size; hv++)
        {
            off = ab_data[hv];
            if (!off)
            {
                walker->idx = 0;
                continue;
            }
            sti = walker->idx;
            edi = ((_short_t *)(data + off))->i;
            the_data = data + off + sizeof(_short_t);
            while (sti < edi)
            {
                *token = (char *)(the_data + (2 * sti));
                if (tlen)
                {
                    *tlen = 2;
                }
                walker->hv = hv;
                walker->idx = sti + 1;
                return 1;
            }
        }
        walker->stage = 3;
        walker->hv = 0;
        walker->idx = 0;
    }

    if (walker->stage == 3)
    {
        for (hv = walker->hv; hv < abc_size; hv++)
        {
            off = abc_data[hv];
            if (!off)
            {
                walker->idx = 0;
                continue;
            }
            sti = walker->idx;
            edi = ((_int_t *)(data + off))->i;
            int *the_int_data = (int *)(data + off + sizeof(_int_t));
            while (sti < edi)
            {
                zcc_msearch_token_t *t = (zcc_msearch_token_t *)(data + the_int_data[sti]);
                *token = (char *)t + sizeof(zcc_msearch_token_t);
                if (tlen)
                {
                    *tlen = t->len;
                }
                walker->hv = hv;
                walker->idx = sti + 1;
                return 1;
            }
            walker->idx = 0;
        }
        walker->stage = -1;
    }
    return 0;
}

zcc_namespace_end;
