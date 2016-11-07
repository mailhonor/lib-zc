/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-09-07
 * ================================
 */

#include "libzc.h"

typedef struct zmap_flat_t zmap_flat_t;
struct zmap_flat_t {
    char *fn;
    zdict_t *dict;
};

static int _close(zmap_node_t * node)
{
    zmap_flat_t *mnode;

    node->used--;
    if (node->used > 0) {
        return 0;
    }

    zgrid_delete(zvar_map_node_list, node->title, 0);

    mnode = (zmap_flat_t *) ((char *)node + sizeof(zmap_node_t));

    if (mnode->dict) {
        zdict_free(mnode->dict);
    }
    zfree(mnode->fn);

    zfree(node);

    return 0;
}

static int _query(zmap_node_t * node, char *query, zbuf_t * result, int timeout)
{
    zmap_flat_t *mnode;
    char *v;
    int len;

    zbuf_reset(result);
    mnode = (zmap_flat_t *) ((char *)node + sizeof(zmap_node_t));

    if (zdict_lookup(mnode->dict, query, &v)) {
        len = strlen(v);
        if (len > 102400) {
            len = 102400;
        }
        if (len > 0) {
            zbuf_memcpy(result, v, len);
            zbuf_terminate(result);
        }
        return 1;
    }

    return 0;
}

int ___load_data(zmap_node_t * node, int flags)
{
    zmap_flat_t *mnode;
    FILE *fp;
    char buf_raw[102400 + 10], *buf, *p;
    int len;

    mnode = (zmap_flat_t *) ((char *)node + sizeof(zmap_node_t));
    fp = fopen(mnode->fn, "r");
    if (!fp) {
        zfatal("zmap_create: %s, can not open (%m)", node->title);
    }
    while ((!feof(fp)) && (!ferror(fp))) {
        if (!fgets(buf_raw, 102400, fp)) {
            break;
        }
        buf = buf_raw;
        while (*buf) {
            if (*buf == ' ' || *buf == '\t') {
                p++;
                continue;
            }
            break;
        }
        if (buf[0] == '#') {
            continue;
        }
        if (buf[0] == 0) {
            continue;
        }
        p = strchr(buf, ' ');
        if (!p) {
            continue;
        }
        *p++ = 0;
        while (*p) {
            if (*p == ' ' || *p == '\t') {
                p++;
                continue;
            }
            break;
        }
        len = strlen(p);
        if ((len > 0) && (p[len - 1] == '\n')) {
            len--;
        }
        if ((len > 0) && (p[len - 1] == '\r')) {
            len--;
        }
        p[len] = 0;
        zdict_add(mnode->dict, buf, p);
    }

    fclose(fp);

    return 0;
}

zmap_node_t *zmap_node_create_flat(char *title, int flags)
{
    zmap_node_t *rnode;
    zmap_flat_t *mnode;

    if (zgrid_lookup(zvar_map_node_list, title, (char **)&rnode)) {
        rnode->used++;
        return rnode;
    }

    rnode = (zmap_node_t *) zcalloc(1, sizeof(zmap_node_t) + sizeof(zmap_flat_t));
    zgrid_add(zvar_map_node_list, title, rnode, 0);
    rnode->close = _close;
    rnode->query = _query;
    rnode->used = 1;
    rnode->title = zstrdup(title);

    mnode = (zmap_flat_t *) ((char *)rnode + sizeof(zmap_node_t));

    mnode->fn = zstrdup(title + 5);
    mnode->dict = zdict_create();

    ___load_data(rnode, flags);

    return rnode;
}
