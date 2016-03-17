/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-15
 * ================================
 */

#include "libzc.h"

int zmail_parser_references_decode(zmail_parser_t * parser, char *refs, zmail_references_t ** list)
{
    zstrtok_t stok;
    zmail_references_t *mid, *fmid, *lmid;

    fmid = 0;
    lmid = 0;
    zstrtok_init(&stok, refs);
    while (zstrtok(&stok, "<> \t,\r\n"))
    {
        if (stok.len < 2)
        {
            continue;
        }
        mid = (zmail_references_t *) zmpool_calloc(parser->mpool, 1, sizeof(zmail_references_t));
        mid->message_id = zmpool_memdup(parser->mpool, stok.str, stok.len);
        if (!fmid)
        {
            fmid = mid;
            lmid = mid;
        }
        else
        {
            lmid->next = mid;
            lmid = mid;
        }
    }

    *list = fmid;

    return 0;
}
