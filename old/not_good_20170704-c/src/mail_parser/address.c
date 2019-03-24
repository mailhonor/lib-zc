/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-08
 * ================================
 */

#include "zc.h"

static int parser_one(zmpool_t *mpool, char **str, int *len, zmime_address_t ** maddr, char *bf)
{
    zmpool_t *imp = mpool;
    char *pstr = *str, c;
    int plen = *len, i, inquote = 0;
    char *name = 0, *mail = 0, last = 0;
    zmime_address_t *ma;

    int bf_idx = 0;
#define  ___put(ch)  { if(bf_idx>10240) return -1;bf[bf_idx++] = (ch);}

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

    bf[bf_idx] = 0;
    pstr = bf;
    plen = bf_idx;
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
        name = ztrim(pstr);
        mail = ztrim(pstr + findi + 1);
    } else {
        name = 0;
        mail = pstr;
    }

    ma = (zmime_address_t *) zmpool_calloc(imp, 1, sizeof(zmime_address_t));
    ma->name = zmpool_strdup(imp, name);
    ma->address = zmpool_strdup(imp, mail);
    ztolower(ma->address);
    ma->name_utf8 = zblank_buffer;

    pstr = name = ma->name;
    while (name && *name) {
        if (*name != '"') {
            *pstr++ = *name++;
        } else {
            name++;
        }
    }
    if (pstr) {
        *pstr = 0;
    }

    *maddr = ma;
#undef ___put
    return 0;
}

const zvector_t *zmime_address_vector_decode_MPOOL(zmpool_t *mpool, const char *in_src, size_t in_len)
{
    zvector_t *r, *rtmp = zvector_create(in_len/6+10);
    zmime_address_t *addr;
    int ret;
    char bf[10249];
    char *str = (char *)in_src;
    int len = (int)in_len;

    while (1) {
        ret = parser_one(mpool, &str, &len, &addr, bf);
        if (ret == -1) {
            break;
        }
        if (ret == -2) {
            continue;
        }
        zvector_add(rtmp, addr);
    }
    r = zvector_create_MPOOL(mpool, ZVECTOR_LEN(rtmp));
    ZVECTOR_WALK_BEGIN(rtmp, addr) {
        zvector_add(r, addr);
    } ZVECTOR_WALK_END;
    zvector_free(rtmp);

    return r;
}

const zvector_t *zmime_address_vector_decode_utf8_MPOOL(zmpool_t *mpool, const char *src_charset_def, const char *in_src, size_t in_len)
{
    zvector_t * r = (zvector_t *)zmime_address_vector_decode_MPOOL(mpool, in_src, in_len);
    zmime_address_t *addr;
    ZSTACK_BUF(dest, 4096);

    ZVECTOR_WALK_BEGIN(r, addr) {
        zbuf_reset(dest);
        zmime_header_line_get_utf8(src_charset_def, addr->name, strlen(addr->name), dest);
        addr->name_utf8 = zmpool_memdupnull(mpool, ZBUF_DATA(dest), ZBUF_LEN(dest));
    } ZVECTOR_WALK_END;
    
    return r;
}

void zmime_address_vector_free(zvector_t *address_vec)
{
    zmime_address_t *addr;
    ZVECTOR_WALK_BEGIN(address_vec, addr) {
        zmpool_free(address_vec->mpool, addr->address);
        zmpool_free(address_vec->mpool, addr->name);
        zmpool_free(address_vec->mpool, addr->name_utf8);
        zmpool_free(address_vec->mpool, addr);
    } ZVECTOR_WALK_END;
    zvector_free(address_vec);
}

/* ********************************************* */
const zvector_t *zmime_address_vector_decode(const char *in_src, size_t in_len)
{
    return zmime_address_vector_decode_MPOOL(0, in_src, in_len);
}

const zvector_t *zmime_address_vector_decode_utf8(const char *src_charset_def, const char *in_src, size_t in_len)
{
    return zmime_address_vector_decode_utf8_MPOOL(0, src_charset_def, in_src, in_len);
}

/* ********************************************* */
const zmime_address_t *zmime_address_decode_MPOOL(zmpool_t *mpool, const char *in_src, size_t in_len)
{
    zmime_address_t *addr;
    int ret;
    char bf[10249];
    char *str = (char *)in_src;
    int len = (int)in_len;

    while (1) {
        ret = parser_one(mpool, &str, &len, &addr, bf);
        if (ret == -1) {
            break;
        }
        if (ret == -2) {
            continue;
        }
        return addr;
    }

    return 0;
}

const zmime_address_t *zmime_address_decode_utf8_MPOOL(zmpool_t *mpool, const char *src_charset_def, const char *in_src, size_t in_len)
{
    zmime_address_t *addr = (zmime_address_t *)zmime_address_decode_MPOOL(mpool, in_src, in_len);
    ZSTACK_BUF(dest, 4096);
    zmime_header_line_get_utf8(src_charset_def, addr->name, strlen(addr->name), dest);
    addr->name_utf8 = zmpool_memdupnull(mpool, ZBUF_DATA(dest), ZBUF_LEN(dest));
    return addr;
}

const zmime_address_t *zmime_address_decode_utf8(const char *src_charset_def, const char *in_src, size_t in_len)
{
    return zmime_address_decode_utf8_MPOOL(0, src_charset_def, in_src, in_len);
}

void zmime_address_free_MPOOL(zmpool_t *mpool, zmime_address_t *addr)
{
    zmpool_free(mpool, addr->address);
    zmpool_free(mpool, addr->name);
    zmpool_free(mpool, addr->name_utf8);
    zmpool_free(mpool, addr);
}

void zmime_address_free(zmime_address_t *addr)
{
    zmime_address_free_MPOOL(0, addr);
}

