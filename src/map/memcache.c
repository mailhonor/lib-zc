/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-09-05
 * ================================
 */

#include "libzc.h"

typedef struct zmap_memcache_t zmap_memcache_t;
struct zmap_memcache_t {
    char *url;
    char *prefix;
    char *suffix;
    pthread_mutex_t locker;
    int fd;
    zstream_t *fp;
};

static int _close(zmap_node_t * node)
{
    zmap_memcache_t *mnode;

    node->used--;
    if (node->used > 0) {
        return 0;
    }

    zgrid_delete(zvar_map_node_list, node->title, 0);

    mnode = (zmap_memcache_t *) ((char *)node + sizeof(zmap_node_t));

    if (mnode->fp) {
        zfclose_FD(mnode->fp);
    }
    if (mnode->fd != -1) {
        close(mnode->fd);
    }
    zfree(mnode->url);
    zfree(mnode->prefix);
    zfree(mnode->suffix);

    zfree(node);

    return 0;
}

static int ___close(zmap_node_t * node)
{
    zmap_memcache_t *mnode;

    mnode = (zmap_memcache_t *) ((char *)node + sizeof(zmap_node_t));
    if (mnode->fp) {
        zfclose_FD(mnode->fp);
        mnode->fp = 0;
    }
    if (mnode->fd != -1) {
        close(mnode->fd);
        mnode->fd = -1;
    }

    return 0;
}

static int ___connect(zmap_node_t * node, int timeout)
{
    zmap_memcache_t *mnode;

    mnode = (zmap_memcache_t *) ((char *)node + sizeof(zmap_node_t));
    if (mnode->fp) {
        return 0;
    }

    if (mnode->fd < 0) {
        mnode->fd = zconnect(mnode->url, timeout);
    }
    if (mnode->fd < 0) {
        return -1;
    }

    mnode->fp = zfopen_FD(mnode->fd);

    return 0;
}

static int ___query(zmap_node_t * node, char *query, zbuf_t * result, int timeout)
{
    int i, ret, rend, len;
    long dtime = ztimeout_set(timeout);
    char buf[102400 + 10];
    zmap_memcache_t *mnode;

    mnode = (zmap_memcache_t *) ((char *)node + sizeof(zmap_node_t));

    for (i = 0; i < 2; i++) {
        zbuf_reset(result);
        if (i) {
            ___close(node);
        }
        ret = ___connect(node, ztimeout_left(dtime));
        if (ret < 0) {
            zbuf_sprintf(result, "zmap_query: %s : connection error", node->title);
            return ret;
        }
        zfset_timeout(mnode->fp, ztimeout_left(dtime));
        sprintf(buf, "get %s%s%s\r\n", mnode->prefix, query, mnode->suffix);
        if ((zfwrite_n(mnode->fp, buf, strlen(buf)) < 0) || (ZFFLUSH(mnode->fp) < 0)) {
            zbuf_sprintf(result, "zmap_query: %s : write error", node->title);
            continue;
        }

        ret = zmap_read_line(mnode->fp, buf, 102400, &rend);
        if (ret < 1) {
            zbuf_sprintf(result, "zmap_query: %s : read error", node->title);
            continue;
        }
        if (!strcmp(buf, "END")) {
            return 0;
        }

        if ((sscanf(buf, "VALUE %*s %*s %d", &len) < 1) || (len < 0)) {
            continue;
        }
        if (len > 102400) {
            zbuf_sprintf(result, "zmap_query: %s : read error, line too long: %d", node->title, len);
            continue;
        }
        if (len > 0) {
            ret = zfread_n(mnode->fp, buf, len);
            if (ret < 1) {
                zbuf_sprintf(result, "zmap_query: %s : read error", node->title);
                continue;
            }
            zbuf_memcpy(result, buf, ret);
            zbuf_terminate(result);
        }

        ret = zmap_read_line(mnode->fp, buf, 102400, &rend);
        if (ret < 0) {
            zbuf_sprintf(result, "zmap_query: %s : read error", node->title);
            continue;
        }
        ret = zmap_read_line(mnode->fp, buf, 102400, &rend);
        if (ret < 0) {
            zbuf_sprintf(result, "zmap_query: %s : read error", node->title);
            continue;
        }
        if (strcmp(buf, "END")) {
            zbuf_sprintf(result, "zmap_query: %s : read error, need END", node->title);
            continue;
        }
        return 1;
    }

    return -1;
}

static int _query(zmap_node_t * node, char *query, zbuf_t * result, int timeout)
{
    zmap_memcache_t *mnode;
    int ret;

    mnode = (zmap_memcache_t *) ((char *)node + sizeof(zmap_node_t));

    if (zvar_map_pthread_mode) {
        zpthread_lock(&(mnode->locker));
    }
    ret = ___query(node, query, result, timeout);
    if (zvar_map_pthread_mode) {
        zpthread_unlock(&(mnode->locker));
    }

    return ret;
}

zmap_node_t *zmap_node_create_memcache(char *title, int flags)
{
    zmap_node_t *rnode;
    zmap_memcache_t *mnode;
    char *args[10] = { "", "", "" };
    char opstr[1024];

    if (zgrid_lookup(zvar_map_node_list, title, (char **)&rnode)) {
        rnode->used++;
        return rnode;
    }

    rnode = (zmap_node_t *) zcalloc(1, sizeof(zmap_node_t) + sizeof(zmap_memcache_t));
    zgrid_add(zvar_map_node_list, title, rnode, 0);
    rnode->close = _close;
    rnode->query = _query;
    rnode->used = 1;
    rnode->title = zstrdup(title);

    mnode = (zmap_memcache_t *) ((char *)rnode + sizeof(zmap_node_t));
    mnode->fd = -1;
    mnode->fp = 0;
    pthread_mutex_init(&(mnode->locker), 0);

    zstrncpy(opstr, title + 9, 1000);
    zmap_title_split(opstr, args);
    if (args[0][0] == 0) {
        zfatal("zmap_create: %s", title);
    }
    mnode->url = zstrdup(args[0]);
    mnode->prefix = zstrdup(args[1]);
    mnode->suffix = zstrdup(args[2]);

    return rnode;
}
