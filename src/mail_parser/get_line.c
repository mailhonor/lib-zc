/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-16
 * ================================
 */

#include "libzc.h"

int zmail_parser_get_body_line(zmail_parser_t * parser, char **ptr)
{
    char *pbegin = parser->mail_pos;
    char *pend = parser->mail_data + parser->mail_size;
    char *p;
    int len;

    *ptr = pbegin;

    len = pend - pbegin;
    if (len < 1) {
        return 0;
    }
    p = memchr(pbegin, '\n', len);

    if (p) {
        len = p - pbegin + 1;
        parser->mail_pos += len;
    } else {
        parser->mail_pos += len;
    }
    return len;
}

int zmail_parser_get_header_line(zmail_parser_t * parser, char **ptr)
{
    char *pbegin = parser->mail_pos;
    char *pend = parser->mail_data + parser->mail_size;
    char *ps, *p;
    int len = 0;

    ps = pbegin;
    while (pend > ps) {
        p = memchr(ps, '\n', pend - ps);
        if ((!p) || (p + 1 == pend)) {
            /* not found or to end */
            len = pend - pbegin;
            break;
        }
        if ((p[1] == ' ') || p[1] == '\t') {
            ps = p + 1;
            continue;
        }
        len = p - pbegin + 1;
        break;
    }

    *ptr = pbegin;

    parser->mail_pos += len;

    return len;
}
