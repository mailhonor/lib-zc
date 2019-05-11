/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-12-08
 * ================================
 */

#include "zc.h"
#include "mime.h"

void zvector_init_mpool(zvector_t *v, int size, zmpool_t *mpool);

int zmime_header_line_address_tok(char **str, int *len, char **rname, char **raddress, char *tmp_cache, int tmp_cache_size)
{
    char *pstr = *str;
    int c;
    int plen = *len, i, inquote = 0;
    char *name = 0, *mail = 0, last = 0;
    int tmp_cache_idx = 0;
#define  ___put(ch)  { if(tmp_cache_idx>tmp_cache_size) return -1;tmp_cache[tmp_cache_idx++] = (ch);}

    if (plen <= 0) {
        return -1;
    }
    for (i = 0; i < plen; i++) {
        c = *(pstr++);
        if (last == '\\') {
            ___put(c);
            last = '\0';
            continue;
        }
        if (c == '\\') {
            last = c;
            continue;
        }
        if (c == '"') {
            if (inquote) {
                inquote = 0;
                ___put(c);
            } else {
                inquote = 1;
            }
            continue;
        }
        if (inquote) {
            ___put(c);
            continue;
        }
        if (c == ',') {
            break;
        }
        ___put(c);
    }
    *len = *len - (pstr - *str);
    *str = pstr;

    tmp_cache[tmp_cache_idx] = 0;
    pstr = tmp_cache;
    plen = tmp_cache_idx;
    if (plen < 1) {
        return -2;
    }
    while (1) {
        pstr = ztrim(pstr);
        plen = strlen(pstr);
        if (plen < 1) {
            return -2;
        }
        if (pstr[plen - 1] == '>') {
            pstr[plen - 1] = ' ';
            continue;
        }
        break;
    }
    unsigned char ch;
    int findi = -1;
    for (i = plen - 1; i >= 0; i--) {
        ch = pstr[i];
        if ((ch == '<') || (ch == ' ') || (ch == '"') || (ch & 0X80)) {
            pstr[i] = 0;
            findi = i;
            break;
        }
    }
    if (findi > -1) {
        name = pstr;
        mail = ztrim(pstr + findi + 1);
    } else {
        name = 0;
        mail = pstr;
    }

    zstr_tolower(mail);
    *raddress = mail;

    char *name_bak = name;
    pstr = name;
    while (name && *name) {
        if (*name != '"') {
            *pstr++ = *name++;
        } else {
            *pstr++ = ' ';
            name++;
        }
    }
    if (pstr) {
        *pstr = 0;
    }
    if (name_bak) {
        int slen = zskip(name_bak, strlen(name_bak), " \t\"'\r\n", 0, rname); 
        if (slen > 0) {
         (*rname)[slen] = 0;
        } else {
            *rname = ztrim(name_bak);
        }
    } else {
        *rname = "";
    }

#undef ___put
    return 0;
}

zvector_t *zmime_header_line_get_address_vector(const char *in_str, int in_len)
{
    if (in_len == -1){
        in_len = strlen(in_str);
    }
    if (in_len < 1) {
        return 0;
    }
    char *n, *a, *str, *cache;
    int len = (int)in_len, ret;
    zvector_t *vec;

    str = (char *)in_str;
    cache = (char *)zmalloc(in_len + 1024);
    vec = zvector_create(10);
    while (1) {
        ret = zmime_header_line_address_tok(&str, &len, &n, &a, cache, in_len + 1000);
        if (ret == -1) {
            break;
        }
        if (ret == -2) {
            continue;
        }
        zmime_address_t *addr = (zmime_address_t *)zmalloc(sizeof(zmime_address_t));
        addr->name = zstrdup(n);
        addr->address = zstrdup(a);
        addr->name_utf8 = zstrdup("");
        zvector_add(vec, addr);
    }
    zfree(cache);

    return vec;
}

zvector_t *zmime_header_line_get_address_vector_inner(zmail_t *parser, const char *in_str, int in_len)
{
    if (in_len == -1){
        in_len = strlen(in_str);
    }
    zmpool_t *mpool = parser->mpool;
    if (in_len < 1) {
        zvector_t *nvec = zmpool_malloc(mpool, sizeof(zvector_t) + sizeof(zmpool_t *));
        zvector_init_mpool(nvec, 0, mpool);
        return nvec;
    }
    char *n, *a, *str, *cache;
    int len = (int)in_len, ret;

    str = (char *)in_str;
    cache = (char *)zmalloc(in_len + 1024);
    zvector_t *vec = zvector_create(128);
    while (1) {
        ret = zmime_header_line_address_tok(&str, &len, &n, &a, cache, in_len + 1000);
        if (ret == -1) {
            break;
        }
        if (ret == -2) {
            continue;
        }
        zmime_address_t *addr = (zmime_address_t *)zmpool_malloc(mpool, sizeof(zmime_address_t));
        addr->name = zmpool_strdup(mpool, n);
        addr->address = zmpool_strdup(mpool, a);
        addr->name_utf8 = zblank_buffer;
        zvector_add(vec, addr);
    }
    zfree(cache);
    zvector_t *nvec = zmpool_malloc(mpool, sizeof(zvector_t) + sizeof(zmpool_t *));
    zvector_init_mpool(nvec, zvector_len(vec), mpool);
    ZVECTOR_WALK_BEGIN(vec, char *, p) {
        zvector_push(nvec, p);
    } ZVECTOR_WALK_END;

    zvector_free(vec);

    return nvec;
}

zvector_t *zmime_header_line_get_address_vector_utf8(const char *src_charset_def, const char *in_str, int in_len)
{
    zvector_t *vec = zmime_header_line_get_address_vector(in_str, in_len);
    if (!vec) {
        return 0;
    }

    zbuf_t *tmpbf = zbuf_create(128);

    ZVECTOR_WALK_BEGIN(vec, zmime_address_t *, addr) {
        if (addr->name[0]) {
            zfree(addr->name_utf8);
            zbuf_reset(tmpbf);
            zmime_header_line_get_utf8(src_charset_def, addr->name, -1, tmpbf);
            addr->name_utf8 = zstrdup(zbuf_data(tmpbf));
        }
    } ZVECTOR_WALK_END;

    zbuf_free(tmpbf);

    return vec;
}


void zmime_header_line_address_vector_free(zvector_t *address_vector)
{
    if (address_vector) {
        ZVECTOR_WALK_BEGIN(address_vector, zmime_address_t *, addr) {
            zfree(addr->name);
            zfree(addr->address);
            zfree(addr->name_utf8);
            zfree(addr);
        } ZVECTOR_WALK_END;
        zvector_free(address_vector);
    }
}

