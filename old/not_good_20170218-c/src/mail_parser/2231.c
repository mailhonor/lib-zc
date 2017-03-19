/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-15
 * ================================
 */

#include "libzc.h"

int zmail_parser_2231_decode(zmail_parser_t * parser, char *in_src, int in_len, zbuf_t *out)
{
    int len, ret;
    char *pend = in_src + in_len;
    char *ps, *p;
    char charset[64];

    if (in_len < 1) {
        return 0;
    }

    p = memchr(in_src, '\'', in_len);
    if (!p) {
        goto err;
    }
    len = p - in_src + 1;
    if (len > 60) {
        len = 60;
    }
    memcpy(charset, in_src, len);
    charset[len] = 0;

    ps = p + 1;
    p = memchr(ps, '\'', pend - ps);
    if (!p) {
        goto err;
    }
    p++;

    {
        char *p;
        p = strchr(charset, '*');
        if (p) {
            *p = 0;
        }
    }
    ret = zmail_parser_iconv(parser, charset, p, pend - p, out);
    if (ret < 1) {
        return 0;
    }
    return ret;
  err:

    zbuf_memcat(out, in_src, in_len);

    return in_len;
}

int zmail_parser_2231_decode_dup(zmail_parser_t * parser, char *in_src, int in_len, char **out_src)
{
    zbuf_t *out;
    int len;

    in_len = zmail_parser_header_value_trim(parser, in_src, in_len, &in_src);
    if (in_len < 1) {
        *out_src = zmpool_memdup(parser->mpool, "", 0);
        return 0;
    }

    if (in_len > ZMAIL_HEADER_LINE_MAX_LENGTH) {
        in_len = ZMAIL_HEADER_LINE_MAX_LENGTH;
    }
    out = zbuf_create(in_len * 3);
    zmail_parser_2231_decode(parser, in_src, in_len, out);

    *out_src = zmpool_memdup(parser->mpool, ZBUF_DATA(out), ZBUF_LEN(out));
    len = ZBUF_LEN(out);
    zbuf_free(out);

    return len;
}
