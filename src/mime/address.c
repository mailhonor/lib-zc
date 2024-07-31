/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-12-08
 * ================================
 */

#include "./mime.h"

static zvector_t *zmime_header_line_get_address_vector_engine(const char *src_charset_def, const char *in_str, int in_len, int loop_mode);

static int zmime_header_line_address_tok(char **str, int *len, char **rname, char **raddress, char *tmp_cache, int tmp_cache_size)
{
    char *pstr = *str;
    int c;
    int plen = *len, i, inquote = 0, find_lt = 0;
    char *name = 0, *mail = 0, last = 0;
    int tmp_cache_idx = 0;
#define ___put(ch)                          \
    {                                       \
        if (tmp_cache_idx > tmp_cache_size) \
            return -1;                      \
        tmp_cache[tmp_cache_idx++] = (ch);  \
    }

    if (plen <= 0)
    {
        return -1;
    }
    for (i = 0; i < plen; i++)
    {
        c = *(pstr++);
        if (last == '\\')
        {
            ___put(c);
            last = '\0';
            continue;
        }
        if (c == '\\')
        {
            last = c;
            continue;
        }
        if (c == '"')
        {
            if (inquote)
            {
                inquote = 0;
                ___put(c);
            }
            else
            {
                inquote = 1;
                find_lt = 0;
            }
            continue;
        }
        if (inquote)
        {
            ___put(c);
            continue;
        }
        if (c == ',')
        {
            break;
        }
        if (c == ';')
        {
            break;
        }
        if (c == '<')
        {
            find_lt = 1;
        }
        ___put(c);
        if (c == '>')
        {
            if (find_lt == 1)
            {
                break;
            }
        }
    }
    *len = *len - (pstr - *str);
    *str = pstr;

    tmp_cache[tmp_cache_idx] = 0;
    pstr = tmp_cache;
    plen = tmp_cache_idx;
    if (plen < 1)
    {
        return -2;
    }
    while (1)
    {
        pstr = ztrim(pstr);
        plen = strlen(pstr);
        if (plen < 1)
        {
            return -2;
        }
        if (pstr[plen - 1] == '>')
        {
            pstr[plen - 1] = ' ';
            continue;
        }
        break;
    }
    unsigned char ch;
    int findi = -1;
    for (i = plen - 1; i >= 0; i--)
    {
        ch = pstr[i];
        if ((ch == '<') || (ch == ' ') || (ch == '"') || (ch & 0X80))
        {
            if ((ch & 0X80) == 0)
            {
                pstr[i] = 0;
            }
            findi = i;
            break;
        }
    }
    if (findi > -1)
    {
        name = pstr;
        mail = ztrim(pstr + findi + 1);
    }
    else
    {
        name = 0;
        mail = pstr;
    }

    *raddress = mail;

    char *name_bak = name;
    pstr = name;
    while (name && *name)
    {
        if (*name != '"')
        {
            *pstr++ = *name++;
        }
        else
        {
            *pstr++ = ' ';
            name++;
        }
    }
    if (pstr)
    {
        *pstr = 0;
    }
    if (name_bak)
    {
        int slen = zskip(name_bak, strlen(name_bak), " \t\"'\r\n", 0, rname);
        if (slen > 0)
        {
            (*rname)[slen] = 0;
        }
        else
        {
            *rname = ztrim(name_bak);
        }
    }
    else
    {
        *rname = "";
    }

#undef ___put
    return 0;
}

static zmime_address_t *_create_mime_address_simple(const char *n, const char *a)
{
    zmime_address_t *addr = (zmime_address_t *)zcalloc(1, sizeof(zmime_address_t));
    addr->name = zstrdup(n);
    addr->name_utf8 = zstrdup("");
    addr->address = zstrdup(a);
    zstr_tolower(addr->address);
    return addr;
}

static zmime_address_t *create_mime_address(const char *src_charset_def, const char *n, const char *a, int loop_mode)
{
    zmime_address_t *addr;
    int alen = 0, i;
    zbuf_t *tmpbf = 0;
    zvector_t *vec = 0;

    if (loop_mode || (n[0]) || (a[0] != '=') || (a[1] != '?'))
    {
        return _create_mime_address_simple(n, a);
    }

    // =?utf-8?B?ImFiYyIgPHh4eEBhYWEuY29tPg==?=
    // "abc" <xxx@aaa.com>

    alen = strlen(a);
    tmpbf = zbuf_create(2 * alen + 100);
    zmime_header_line_get_utf8(src_charset_def, a, alen, tmpbf);
    vec = zmime_header_line_get_address_vector_engine(src_charset_def, zbuf_data(tmpbf), zbuf_len(tmpbf), 1);
    if ((!vec) || (!zvector_len(vec)))
    {
        zbuf_free(tmpbf);
        return _create_mime_address_simple(n, a);
    }
    zbuf_reset(tmpbf);
    i = 0;
    ZVECTOR_WALK_BEGIN(vec, zmime_address_t *, ma)
    {
        if (i)
        {
            zbuf_strcat(tmpbf, " ");
        }
        zbuf_strcat(tmpbf, ma->name);
        i = 1;
    }
    ZVECTOR_WALK_END;

    addr = (zmime_address_t *)zcalloc(1, sizeof(zmime_address_t));
    addr->name = zstrdup(zbuf_data(tmpbf));
    addr->name_utf8 = zstrdup(zblank_buffer);
    addr->address = zstrdup(((zmime_address_t *)(vec->data[0]))->address);

    zbuf_free(tmpbf);
    zmime_header_line_address_vector_free(vec);

    return addr;
}

static zvector_t *zmime_header_line_get_address_vector_engine(const char *src_charset_def, const char *in_str, int in_len, int loop_mode)
{
    if (in_len == -1)
    {
        in_len = strlen(in_str);
    }
    if (in_len < 1)
    {
        return zvector_create(1);
    }
    char *n, *a, *str, *cache;
    int len = (int)in_len, ret;
    zvector_t *vec;

    str = (char *)in_str;
    cache = (char *)zmalloc(in_len + 1024);
    vec = zvector_create(10);
    while (1)
    {
        ret = zmime_header_line_address_tok(&str, &len, &n, &a, cache, in_len + 1000);
        if (ret == -1)
        {
            break;
        }
        if (ret == -2)
        {
            continue;
        }
        zmime_address_t *addr = create_mime_address(src_charset_def, n, a, 0);
        zvector_add(vec, addr);
    }
    zfree(cache);

    return vec;
}

zvector_t *zmime_header_line_get_address_vector(const char *src_charset_def, const char *in_str, int in_len)
{
    return zmime_header_line_get_address_vector_engine(src_charset_def, in_str, in_len, 0);
}

zvector_t *zmime_header_line_get_address_vector_utf8(const char *src_charset_def, const char *in_str, int in_len)
{
    zvector_t *vec = zmime_header_line_get_address_vector(src_charset_def, in_str, in_len);
    zbuf_t *tmpbf = zbuf_create(128);
    zbuf_t *bq_join = zbuf_create(-1);
    zbuf_t *out_string = zbuf_create(-1);

    ZVECTOR_WALK_BEGIN(vec, zmime_address_t *, addr)
    {
        if (addr->name[0] && (!addr->name_utf8[0]))
        {
            zfree(addr->name_utf8);
            zbuf_reset(tmpbf);
            zbuf_reset(bq_join);
            zbuf_reset(out_string);
            zmime_header_line_get_utf8_engine(src_charset_def, addr->name, -1, tmpbf, bq_join, out_string);
            addr->name_utf8 = zstrdup(zbuf_data(tmpbf));
        }
    }
    ZVECTOR_WALK_END;

    zbuf_free(tmpbf);
    zbuf_free(bq_join);
    zbuf_free(out_string);

    return vec;
}

void zmime_address_free(zmime_address_t *addr)
{
    if (!addr)
    {
        return;
    }
    zfree(addr->name);
    zfree(addr->address);
    zfree(addr->name_utf8);
    zfree(addr);
}

void zmime_header_line_address_vector_free(zvector_t *address_vector)
{
    if (address_vector)
    {
        ZVECTOR_WALK_BEGIN(address_vector, zmime_address_t *, addr)
        {
            zmime_address_free(addr);
        }
        ZVECTOR_WALK_END;
        zvector_free(address_vector);
    }
}
