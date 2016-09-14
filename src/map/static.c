/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-09-04
 * ================================
 */

#include "libzc.h"

static int _close(zmap_node_t *node)
{
    zfree(node->title);
    zfree(node);

    return 0;
}

static int _query(zmap_node_t * node, char *query, zbuf_t *result, int timeout)
{
    zbuf_strcpy(result, (char *)(node->title));
    return 1;
}

zmap_node_t *zmap_node_create_static(char *title, int flags)
{
    zmap_node_t *rnode;

    rnode = zcalloc(1, sizeof(zmap_node_t));
    rnode->close = _close;
    rnode->query = _query;
    rnode->title = (void *)zstrdup(title + 7);

    return rnode;
}
