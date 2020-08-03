/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2020-07-27
 * ================================
 */

#include "zc.h"

#define _MAX_LINE_SIZE  4096

#define _CODE_ID        "ZMSH"
#define _CODE_VERSION   "0001"

zbool_t zvar_msearch_error_msg = 0;

typedef struct _short_t _short_t;
typedef struct _int_t _int_t;
typedef struct zmsearch_token_t zmsearch_token_t;
typedef struct zmsearch_engine_t zmsearch_engine_t;
typedef struct zmsearch_builder_t zmsearch_builder_t;

struct _short_t {
    short int i;
};
struct _int_t {
    int i;
};

struct zmsearch_token_t {
    short int len;
    /* void *data */
};

struct zmsearch_builder_t {
    char a_data[256];
    zmap_t *ab_data_tmp;
    zmap_t *abc_data_tmp;
    unsigned char *tmpkey;
    unsigned char *mem_data;
    int mem_len;
    int mem_capability;
};

struct zmsearch_engine_t {
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
    /* offset ==> (_int_t + n * (zmsearch_token_t *)) */
    int abc_data_offset;
    int abc_size;
};

struct zmsearch_t {
    zmsearch_engine_t *engine;
    zmsearch_builder_t *builder;
};

zmsearch_t *zmsearch_create()
{
    zmsearch_t *ms = (zmsearch_t *)zcalloc(1, sizeof(zmsearch_t));
    ms->builder = (zmsearch_builder_t *)zcalloc(1, sizeof(zmsearch_builder_t));
    ms->builder->ab_data_tmp = zmap_create();
    ms->builder->abc_data_tmp = zmap_create();
    ms->builder->tmpkey = (unsigned char *)malloc(_MAX_LINE_SIZE+1);
    return ms;
}

static void zmsearch_free_builder(zmsearch_t *ms)
{
    zmsearch_builder_t *builder = ms->builder;
    if (!builder) {
        return;
    }
    if (builder->ab_data_tmp) {
        zmap_free(builder->ab_data_tmp);
        builder->ab_data_tmp = 0;
    }
    if (builder->abc_data_tmp) {
        zmap_free(builder->abc_data_tmp);
        builder->abc_data_tmp = 0;
    }
    ZFREE(builder->mem_data);
    ZFREE(builder->tmpkey);
    zfree(builder);
    ms->builder = 0;
}

void zmsearch_free(zmsearch_t *ms)
{
    if (!ms) {
        return;
    }
    if (ms->builder == (zmsearch_builder_t *)-1) {
    } else if (ms->builder == (zmsearch_builder_t *)-2) {
        zmmap_reader_t *reader = (zmmap_reader_t *)((char *)ms + sizeof(zmsearch_t));
        zmmap_reader_fini(reader);
    } else {
        zmsearch_free_builder(ms);
        if (ms->engine) {
            zfree(ms->engine);
            ms->engine = 0;
        }
    }
    zfree(ms);
}

void zmsearch_add_token(zmsearch_t *ms, const char *word, int len)
{
    if (!ms->builder) {
        zfatal("zmsearch_add_over already excuted");
    }
    if (len < 0) {
        len = strlen(word);
    }
    if (len < 1) {
        return;
    }
    if (len > _MAX_LINE_SIZE) {
        return;
    }
    zmsearch_builder_t *builder = ms->builder;
    unsigned char *key = builder->tmpkey;
    unsigned char *data = (unsigned char *)word;
    if (len == 1) {
        builder->a_data[data[0]] |= 0X01;
        return;
    }
    if (len == 2) {
        builder->a_data[data[0]] |= 0X02;
        builder->a_data[data[1]] |= 0X04;
        key[0] = data[0];
        key[1] = data[1];
        key[2] = 0;
        zmap_update(builder->ab_data_tmp, (char *)key, 0, 0);
        return;
    }
    builder->a_data[data[0]] |= 0X08;
    builder->a_data[data[1]] |= 0X10;
    builder->a_data[data[2]] |= 0X20;
    memcpy(key, data, len);
    key[len] = 0;
    zmap_update(builder->abc_data_tmp, (char *)key, 0, 0);
}

int _mem_malloc(zmsearch_builder_t *m, int len)
{
    int changed = 0;
    while (m->mem_capability - m->mem_len < len) {
        m->mem_capability = 2 * m->mem_capability + 1;
        changed = 1;
    }
    if (changed) {
        m->mem_data = (unsigned char *)zrealloc(m->mem_data, m->mem_capability + 1);
    }
    memset(m->mem_data + m->mem_len, 0, len);
    int r = m->mem_len;
    m->mem_len += len;
    return r;
}

static void zmsearch_add_over_1(zmsearch_t *ms)
{
    zmsearch_builder_t *builder = ms->builder;
    if (builder == 0) {
        zfatal("zmsearch_add_over already be excuted");
    }
    if (((zmsearch_builder_t *)-10 < builder) && (builder < 0)) {
        zfatal("zmsearch_add_over should not be excuted");
    }

    builder->mem_capability = 1024 * 1024;
    builder->mem_data = (unsigned char *)malloc(builder->mem_capability + 1);

    unsigned char **data = &(builder->mem_data);
    zmsearch_engine_t **engine = (zmsearch_engine_t **)data;
    _mem_malloc(builder, sizeof(zmsearch_engine_t));
    memcpy((*engine)->a_data, builder->a_data, 256);
}

static void zmsearch_add_over_2(zmsearch_t *ms)
{
    zmsearch_builder_t *builder = ms->builder;
    unsigned char **data = &(builder->mem_data);
    zmsearch_engine_t **engine = (zmsearch_engine_t **)data;

    int size = zmap_len(builder->ab_data_tmp);
    if (!size) {
        return;
    }
    size = 2 * size + 1;
    (*engine)->ab_size = size;
    (*engine)->ab_data_offset = _mem_malloc(builder, sizeof(int) * size);
    ZMAP_WALK_BEGIN(builder->ab_data_tmp, word, void *, tmpv) {
        int hv = (unsigned char )word[0];
        hv = (hv << 8) + (unsigned char)word[1];
        int *ab_data_ptr = (int *)((*data) + (*engine)->ab_data_offset);
        ab_data_ptr[hv%size]++;
    } ZMAP_WALK_END;
    for (int i = 0; i < size; i++) {
        int *ab_data_ptr = (int *)((*data) + (*engine)->ab_data_offset);
        if (ab_data_ptr[i]) {
            ab_data_ptr[i] = _mem_malloc(builder, sizeof(_short_t) + 2 * ab_data_ptr[i]);
            _short_t *_s = (_short_t *)((*data) + ab_data_ptr[i]);
            _s->i = 0;
        }
    }
    ZMAP_WALK_BEGIN(builder->ab_data_tmp, word, void *, tmpv) {
        int hv = (unsigned char )word[0];
        hv = (hv << 8) + (unsigned char)word[1];
        int *ab_data_ptr = (int *)((*data) + (*engine)->ab_data_offset);
        _short_t *_s = (_short_t *)((*data) + ab_data_ptr[hv%size]);
        unsigned char *p12 = ((unsigned char *)_s) + sizeof(_short_t) + 2 * _s->i;
        p12[0] = (unsigned char )word[0];
        p12[1] = (unsigned char )word[1];
        _s->i++;
    } ZMAP_WALK_END;
}

static void zmsearch_add_over_3_clear(zmsearch_t *ms)
{
    zmsearch_builder_t *builder = ms->builder;

    int size = zmap_len(builder->abc_data_tmp);
    if (!size) {
        return;
    }
    zmap_t *del_keys_map = zmap_create();
    const char *last = 0;
    int llen;
    ZMAP_WALK_BEGIN(builder->abc_data_tmp, word, void *, tmpv) {
        if (last  == 0) {
            last = word;
            llen = strlen(last);
        } else {
            if (!strncmp(last, word, llen)) {
                zmap_update(del_keys_map, word, 0, 0);
            } else {
                last = word;
                llen = strlen(last);
            }
        }
    } ZMAP_WALK_END;

    ZMAP_WALK_BEGIN(del_keys_map, word, void *, tmpv) {
        zmap_delete(builder->abc_data_tmp, word, 0);
    } ZMAP_WALK_END;

    zmap_free(del_keys_map);
}

static void zmsearch_add_over_3(zmsearch_t *ms)
{
    int tmpoffset;
    zmsearch_builder_t *builder = ms->builder;
    unsigned char **data = &(builder->mem_data);
    zmsearch_engine_t **engine = (zmsearch_engine_t **)data;

    int size = zmap_len(builder->abc_data_tmp);
    if (!size) {
        return;
    }
    size = 2 * size + 1;
    (*engine)->abc_size = size;
    tmpoffset = _mem_malloc(builder, sizeof(int) * size);
    (*engine)->abc_data_offset = tmpoffset;
    ZMAP_WALK_BEGIN(builder->abc_data_tmp, word, void *, tmpv) {
        int hv = (unsigned char )word[0];
        hv = (hv << 8) + (unsigned char)word[1];
        hv = (hv << 8) + (unsigned char)word[2];
        int *abc_data_ptr = (int *)((*data) + (*engine)->abc_data_offset);
        abc_data_ptr[hv%size]++;
    } ZMAP_WALK_END;
    for (int i = 0; i < size; i++) {
        int *abc_data_ptr = (int *)((*data) + (*engine)->abc_data_offset);
        if (abc_data_ptr[i]) {
            tmpoffset = _mem_malloc(builder, sizeof(_int_t) + sizeof(int) * abc_data_ptr[i]);
            abc_data_ptr[i] = tmpoffset;
            _int_t *_i = (_int_t *)((*data) + abc_data_ptr[i]);
            _i->i = 0;
        }
    }
    ZMAP_WALK_BEGIN(builder->abc_data_tmp, word, void *, tmpv) {
        int hv = (unsigned char )word[0];
        hv = (hv << 8) + (unsigned char)word[1];
        hv = (hv << 8) + (unsigned char)word[2];
        int len = strlen(word);
        int offset = _mem_malloc(builder, sizeof(zmsearch_token_t) + len);
        int *abc_data_ptr = (int *)((*data) + (*engine)->abc_data_offset);
        _int_t *_i = (_int_t *)((*data) + abc_data_ptr[hv%size]);
        *((int *)(((char *)_i) + sizeof(_int_t) + sizeof(int) * (_i->i))) = offset;
        zmsearch_token_t *token = (zmsearch_token_t *)((*data) + offset);
        token->len = len;
        memcpy(((char *)token) + sizeof(zmsearch_token_t), word, len);
        _i = (_int_t *)((*data) + abc_data_ptr[hv%size]);
        _i->i++;
    } ZMAP_WALK_END;
}

static void zmsearch_add_over_finished(zmsearch_t *ms)
{
    zmsearch_builder_t *builder = ms->builder;
    ms->engine = (zmsearch_engine_t *)zmemdupnull(builder->mem_data, builder->mem_len);
    ms->engine->data_len = builder->mem_len;
    memcpy(ms->engine->code_id, _CODE_ID, 4);
    memcpy(ms->engine->code_version, _CODE_VERSION, 4);
    zmsearch_free_builder(ms);
}

void zmsearch_add_over(zmsearch_t *ms)
{
    zmsearch_add_over_1(ms);
    zmsearch_add_over_2(ms);
    zmsearch_add_over_3_clear(ms);
    zmsearch_add_over_3(ms);
    zmsearch_add_over_finished(ms);
}

int zmsearch_match(zmsearch_t *ms, const char *str, int len, int *offset)
{
    if (len < 0) {
        len = strlen(str);
    }
    if (len < 1) {
        return 0;
    }

    zmsearch_engine_t *engine = ms->engine;
    if (!engine) {
        zfatal("should excute zmsearch_add_over first");
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

    for (int i=0;i<len;i++) {
        ps = ustr + i;
        plen = len - i;
        if (plen < 1) {
            return 0;
        } 
        if (offset) {
            *offset = i;
        }
        firstch = a_data[ps[0]];
        if (firstch & 0X01) {
            return 1;
        }
        while ((plen>1) && (firstch & 0X02) && (a_data[ps[1]] & 0X04)) {
            hv = ps[0];
            hv = (hv << 8) + ps[1];
            off = ab_data[hv%ab_size];
            if (!off) {
                break;
            }
            sti = 0;
            edi = ((_short_t *)(data + off))->i;
            the_data = data + off + sizeof(_short_t);
            while (sti < edi) {
                mdi = (sti+edi)/2;
                v = the_data[2 * mdi];
                v = (v << 8) + the_data[2*mdi+1];
                if (hv == v) {
                    return 2;
                }
                if (hv < v) {
                    edi--;
                    continue;
                }
                if (v < hv) {
                    sti++;
                    continue;
                }
            }
            break;
        }
        if ((plen>2) && (firstch & 0X08) && (a_data[ps[1]] & 0X10) && (a_data[ps[2]] & 0x20)) {
        } else {
            continue;
        }

        hv = ps[0];
        hv = (hv << 8) + ps[1];
        hv = (hv << 8) + ps[2];
        off = abc_data[hv%abc_size];
        if (!off) {
            continue;
        }
        sti = 0;
        edi = ((_int_t *)(data + off))->i;
        int *the_int_data = (int *)(data + off + sizeof(_int_t));
        while (sti < edi) {
            mdi = (sti+edi)/2;
            zmsearch_token_t *token = (zmsearch_token_t *)(data + the_int_data[mdi]);
            int tlen = token->len;
            const void *tdata = (const char *)token + sizeof(zmsearch_token_t);
            if (plen < tlen) {
                int r = memcmp(ps, tdata, plen);
                if (r <= 0) {
                    edi = mdi;
                    continue;
                } else {
                    sti = mdi+1;
                    continue;
                }
            } else {
                int r = memcmp(ps, tdata, tlen);
                if (r == 0) {
                    return tlen;
                } else if (r < 0) {
                    edi = mdi;
                    continue;
                } else {
                    sti = mdi+1;
                    continue;
                }
            }
        }
    }
    return 0;
}

int zmsearch_add_token_from_pathname(zmsearch_t *ms, const char *pathname)
{
    char buf[4096+1];
    FILE *fp = fopen(pathname, "r");
    if (!fp) {
        return -1;
    }
    while(fgets(buf, 4096, fp)) {
        char *ps = buf;
        ps = ztrim(ps);
        if (*ps == 0) {
            continue;
        }
        if (!strncmp(ps, "###", 3)) {
            continue;
        }
        zmsearch_add_token(ms, ps, -1);
    }
    fclose(fp);
    return 1;
}

zmsearch_t *zmsearch_create_from_data(const void *data)
{
    if (memcmp(data, _CODE_ID, 4)) {
        if (zvar_msearch_error_msg) {
            zinfo("ERR zmsearch_t invalid data");
        }
        return 0;
    }

    if (memcmp((char *)data + 4, _CODE_VERSION, 4)) { 
        if (zvar_msearch_error_msg) {
            zinfo("ERR zmsearch_t version mismatched");
        }
        return 0;
    }
    zmsearch_t *ms = (zmsearch_t *)zcalloc(1, sizeof(zmsearch_t));
    ms->engine = (zmsearch_engine_t *)data;
    ms->builder = (zmsearch_builder_t *)-1;
    return ms;
}

zmsearch_t *zmsearch_create_from_pathname(const char *pathname)
{
    zmsearch_t *ms = (zmsearch_t *)zcalloc(1, sizeof(zmsearch_t) + sizeof(zmmap_reader_t));
    zmmap_reader_t *reader = (zmmap_reader_t *)((char *)ms + sizeof(zmsearch_t));
    if (zmmap_reader_init(reader, pathname) < 1) {
        zfree(ms);
        return 0;
    }
    if (reader->len < 8) {
        zmmap_reader_fini(reader);
        zfree(ms);
        if (zvar_msearch_error_msg) {
            zinfo("ERR zmsearch_t invalid data");
        }
        return 0;
    }
    if (memcmp(reader->data, _CODE_ID, 4)) {
        zmmap_reader_fini(reader);
        zfree(ms);
        if (zvar_msearch_error_msg) {
            zinfo("ERR zmsearch_t invalid data");
        }
        return 0;
    }

    if (memcmp((char *)(reader->data) + 4, _CODE_VERSION, 4)) { 
        zmmap_reader_fini(reader);
        zfree(ms);
        if (zvar_msearch_error_msg) {
            zinfo("ERR zmsearch_t version mismatched");
        }
        return 0;
    }
    ms->engine = (zmsearch_engine_t *)(reader->data);
    ms->builder = (zmsearch_builder_t *)-2;
    return ms;
}

const void *zmsearch_get_compiled_data(zmsearch_t *ms)
{
    return ms->engine;
}

int zmsearch_get_compiled_len(zmsearch_t *ms)
{
    if (!ms->engine) {
        return 0;
    }
    return ms->engine->data_len;
}
