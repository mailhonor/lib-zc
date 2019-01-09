/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "zc.h"
#include "mime.h"

/* clear null characters */

char *zmail_clear_null_inner(const void *data, int size)
{
    char *p = (char *)(void *)data;
    int i;
    for (i=0;i<size;i++) {
        if (p[i] == '\0') {
            p[i] = ' ';
        }
    }
    return (char *)(void *)data;
}

/* cache */
zbuf_t *zmail_zbuf_cache_require(zmail_t *parser, int len)
{
    zbuf_node_t *n = parser->zbuf_cache_tail;
    if (n) {
        ZMLINK_DETACH(parser->zbuf_cache_head, parser->zbuf_cache_tail, n, prev, next);
    } else {
        n = (zbuf_node_t *)zmalloc(sizeof(zbuf_node_t));
        zbuf_init(&(n->buf), len);
    }
    zbuf_reset(&(n->buf));
    return &(n->buf);
}

void zmail_zbuf_cache_release(zmail_t *parser, zbuf_t *bf)
{
    if (!bf) {
        return;
    }
    zbuf_node_t *n = ZCONTAINER_OF(bf, zbuf_node_t, buf);
    ZMLINK_APPEND(parser->zbuf_cache_head, parser->zbuf_cache_tail, n, prev, next);
}

void zmail_zbuf_cache_release_all(zmail_t *parser)
{
    while(parser->zbuf_cache_head) {
        zbuf_node_t *n = parser->zbuf_cache_head;
        ZMLINK_DETACH(parser->zbuf_cache_head, parser->zbuf_cache_tail, n, prev, next);
        zbuf_fini(&(n->buf));
        zfree(n);
    }
}
