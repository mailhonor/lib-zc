/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-08
 * ================================
 */

#include "libzc.h"

static int parser_one(zmail_parser_t * parser, char **str, int *len, zmail_addr_t ** maddr, char *bf)
{
    zmpool_t *imp = parser->mpool;
    char *pstr = *str, c;
    int plen = *len, i, inquote = 0;
    char *name = 0, *mail = 0, last = 0;
    zmail_addr_t *ma;

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

    ma = (zmail_addr_t *) zmpool_calloc(imp, 1, sizeof(zmail_addr_t));
    ma->name = zmpool_strdup(imp, name);
    ma->mail = zmpool_strdup(imp, mail);
    ztolower(ma->mail);

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

int zmail_parser_addr_decode(zmail_parser_t * parser, char *str, int len, zmail_addr_t ** maddr)
{
    zmail_addr_t *faddr, *laddr, *naddr;
    int ret;
    char bf[10249];

    faddr = 0;
    laddr = 0;
    while (1) {
        ret = parser_one(parser, &str, &len, &naddr, bf);
        if (ret == -1) {
            break;
        }
        if (ret == -2) {
            continue;
        }
        if (laddr) {
            laddr->next = naddr;
            laddr = naddr;
        } else {
            faddr = naddr;
            laddr = naddr;
        }
    }

    *maddr = faddr;

    /* decode name which be limited within 4096 */
    if (!zmail_parser_only_test_parse) {
        int name_len;
        for (; faddr; faddr = faddr->next) {
            name_len = strlen(faddr->name);
            if (name_len > 4096) {
                name_len = 4096;
            }
            zmail_parser_header_value_decode_dup(parser, faddr->name, name_len, &(faddr->name_rd));
        }
    }

    return 0;
}

void zmail_parser_addr_free(zmail_parser_t * parser, zmail_addr_t * ma)
{
    zmail_addr_t *mn;
#define ___mf(a)	{if(ma->a)zmpool_free(parser->mpool, ma->a);}
    while (ma) {
        mn = ma->next;
        ___mf(name);
        ___mf(mail);
        ___mf(adl);
        ___mf(name_rd);
        zmpool_free(parser->mpool, ma);
        ma = mn;
    }
#undef ___mf
}
