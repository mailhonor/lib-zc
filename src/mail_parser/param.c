/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-16
 * ================================
 */

#include "libzc.h"

#define ___CASEEQ_LEN(a, b, n)          ((zchar_toupper(a[0]) == zchar_toupper((b)[0])) && (!strncasecmp(a,b,n)))
#define ___CASEEQ(a, b)                 ((zchar_toupper(a[0]) == zchar_toupper(b[0])) && (!strcasecmp(a,b)))
#define ___EQ_LEN(a, b, n)              ((a[0] == b[0]) && (!strncmp(a,b,n)))

static char *ignore_chs(char *p, int plen, char *chs, int len, int flag)
{
    int i = 0, j;
    if (plen < 1)
        return (NULL);
    if (flag < 0)
        p += plen - 1;
    for (i = 0; i < plen; i++)
    {
        for (j = 0; j < len; j++)
        {
            if (*(p) == chs[j])
                break;
        }
        if (j == len)
            return (p);
        p += flag;
    }
    return (NULL);
}

static char *find_delim(char *p, int plen, char *chs, int len)
{
    int i, j;
    for (i = 0; i < plen; i++)
    {
        for (j = 0; j < len; j++)
        {
            if (p[i] == chs[j])
                return (p + i);
        }
    }
    return (NULL);
}

/* ################################################################## */
static int find_next_kv(char *buf, int len, char **key, int *key_len, char **value, int *value_len, char **nbuf, int *nlen)
{
    char *p = buf, *p1, *p2, *pe, *pn = 0;
    int find;

    p = ignore_chs(p, len, ";\t \r\n", 4, 1);
    if (!p)
    {
        return -1;
    }
    *nbuf = 0;
    *key = p;

    pe = memchr(p, '=', len - (p - buf));
    if (!pe)
    {
        return -1;
    }
    *pe = 0;

    p1 = find_delim(p, pe - p, "\t \r\n", 4);
    if (p1)
    {
        *p1 = 0;
    }
    *key_len = strlen(p);

    p = pe + 1;
    p = ignore_chs(p, len - (p - buf), "\t \r\n", 4, 1);
    if (!p)
    {
        return -1;
    }
    find = 0;
    if (*p == '"')
    {
        p++;
        find = 1;
    }
    *value = p;
    if (find)
    {
        p2 = find_delim(p, len - (p - buf), "\"\r\n", 3);
    }
    else
    {
        p2 = find_delim(p, len - (p - buf), "\t ;\r\n", 5);
    }
    if (p2)
    {
        *value_len = p2 - p;
        pn = p2 + 1;
    }
    else
    {
        *value_len = strlen(p);
    }

    if (pn)
    {
        *nbuf = pn;
        *nlen = len - (pn - buf);
    }

    return 0;
}

int deal_kv_list(zmail_parser_t * parser, zmail_mime_t * cmime, char *buf, int len)
{
    char *p, *key, *value, *nbuf;
    int l, key_len, value_len, nlen = 0;
    int ret;
    char fns[10249];
    int fns_len = 0;
    int fns_type = 0;           /* 0: none charset, 1: have charset */

    p = buf;
    l = len;

#define _FR(a)	{if(cmime->a) zmpool_free(parser->mpool, cmime->a);cmime->a=0;}
    while (1)
    {
        if (l < 2)
        {
            break;
        }
        ret = find_next_kv(p, l, &key, &key_len, &value, &value_len, &nbuf, &nlen);
        if (ret)
        {
            break;
        }
        if (___CASEEQ_LEN(key, "boundary", key_len))
        {
            _FR(boundary);
            cmime->boundary = (char *)zmpool_malloc(parser->mpool, value_len + 3);
            cmime->boundary[0] = '-';
            cmime->boundary[1] = '-';
            memcpy(cmime->boundary + 2, value, value_len);
            cmime->boundary[value_len + 2] = 0;
            cmime->boundary_len = value_len + 2;
        }
        else if (___CASEEQ_LEN(key, "charset", key_len))
        {
            _FR(charset);
            zmail_parser_header_value_dup(parser, value, value_len, &(cmime->charset));
        }
        else if (___CASEEQ_LEN(key, "name", key_len))
        {
            _FR(name);
            int rlen = zmail_parser_header_value_dup(parser, value, value_len,
                                                     &(cmime->name));
            if ((rlen > 0) && (!zmail_parser_only_test_parse))
            {
                _FR(name_rd);
                zmail_parser_header_value_decode_dup(parser, cmime->name, rlen, &(cmime->name_rd));
            }
        }
        else if (___CASEEQ_LEN(key, "filename", key_len))
        {
            _FR(filename);
            int rlen = zmail_parser_header_value_dup(parser, value, value_len,
                                                     &(cmime->filename));
            if ((rlen > 0) && (!zmail_parser_only_test_parse))
            {
                _FR(filename_rd);
                zmail_parser_header_value_decode_dup(parser, cmime->filename, rlen, &(cmime->filename_rd));
            }
        }
        else if ((key_len > 8) && (___CASEEQ_LEN(key, "filename*", 9)))
        {
            /* rfc 2231 */
            char *ps, *p;
            ps = key + 9;
            p = ps;
            if (*p == '0')
            {
                p++;
                if (*p == '*')
                {
                    fns_type = 1;
                }
                else
                {
                    fns_type = 0;
                }
                p--;
            }
            if (fns_len + value_len < 10240)
            {
                memcpy(fns + fns_len, value, value_len);
                fns_len += value_len;
            }
        }
        if (nbuf == 0)
        {
            break;
        }
        p = nbuf;
        l = nlen;
    }

    if (fns_len > 0)
    {
        if (fns_type == 0)
        {
            _FR(filename);
            int rlen = zmail_parser_header_value_dup(parser, fns, fns_len, &(cmime->filename));
            if ((rlen > 0) && (!zmail_parser_only_test_parse))
            {
                _FR(filename_rd);
                zmail_parser_header_value_decode_dup(parser, cmime->filename, rlen, &(cmime->filename_rd));
            }
        }
        else
        {
            _FR(filename_star);
            int rlen = zmail_parser_header_value_dup(parser, fns, fns_len, &(cmime->filename_star));
            if ((rlen > 0) && (!zmail_parser_only_test_parse))
            {
                _FR(filename_rd);
                zmail_parser_2231_decode_dup(parser, cmime->filename_star, rlen, &(cmime->filename_rd));
            }
        }
    }
#undef _FR

    return 0;
}

static int find_value(char *buf, int len, char **value, int *value_len, char **nbuf, int *nlen)
{
    char *p = buf, *p1;

    p = ignore_chs(p, len, "\t \"", 3, 1);
    if (!p)
    {
        return -1;
    }
    *nbuf = 0;
    *value = p;
    p1 = find_delim(p, len - (p - buf), ";\t \"\r\n", 6);
    if (p1)
    {
        *value_len = p1 - p;
        *nbuf = p1 + 1;
        *nlen = len - (p1 + 1 - buf);
    }
    else
    {
        *value_len = len - (p - buf);
    }

    return 0;
}

int zmail_parser_header_parse_param(zmail_parser_t * parser, zmail_mime_t * cmime, char *buf, int len, char **attr)
{
    char *value, *nbuf;
    int value_len, nlen;

    if (find_value(buf, len, &value, &value_len, &nbuf, &nlen))
    {
        return -1;
    }
    zmail_parser_header_value_dup(parser, value, value_len, attr);

    if (nbuf == 0)
    {
        return 0;
    }
    deal_kv_list(parser, cmime, nbuf, nlen);

    return 0;
}
