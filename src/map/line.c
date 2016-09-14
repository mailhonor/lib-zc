/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-09-07
 * ================================
 */

#include "libzc.h"

typedef struct zmap_line_t zmap_line_t;
struct zmap_line_t {
    char *url;
    pthread_mutex_t locker;
    int fd;
    zstream_t *fp;
};

static int _close(zmap_node_t *node)
{
    zmap_line_t *mnode;

    node->used --;
    if (node->used > 0) {
        return 0;
    }
    
    zgrid_delete(zvar_map_node_list, node->title, 0);

    mnode = (zmap_line_t *)((char *)node + sizeof(zmap_node_t));

    if(mnode->fp) {
        zfclose_FD(mnode->fp);
    }
    if(mnode->fd != -1) {
        close(mnode->fd);
    }
    zfree(mnode->url);

    zfree(node);

    return 0;
}

static int ___close(zmap_node_t *node)
{
    zmap_line_t *mnode;
   
    mnode = (zmap_line_t *)((char *)node + sizeof(zmap_node_t));
    if(mnode->fp) {
        zfclose_FD(mnode->fp);
        mnode->fp = 0;
    }
    if (mnode->fd != -1) {
        close(mnode->fd);
        mnode->fd = -1;
    }

    return 0;
}

static int ___connect(zmap_node_t *node, int timeout)
{
    zmap_line_t *mnode;
   
    mnode = (zmap_line_t *)((char *)node + sizeof(zmap_node_t));
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

static int ___query(zmap_node_t * node, char *query, zbuf_t *result, int timeout)
{
    int i, ret, rend, len;
    long dtime = ztimeout_set(timeout);
    char buf[102400 + 10];
    zmap_line_t *mnode;
   
    mnode = (zmap_line_t *)((char *)node + sizeof(zmap_node_t));

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
		sprintf(buf, "%s\r\n", query);
        if ((zfwrite_n(mnode->fp, buf, strlen(buf)) < 0) || (ZFFLUSH(mnode->fp) < 0)) {
            zbuf_sprintf(result, "zmap_query: %s : write error", node->title);
            continue;
        }

        len = zmap_read_line(mnode->fp, buf, 102400, &rend);
        if (len > -1) {
            if (len > 0) {
                zbuf_memcpy(result, buf, len);
                zbuf_terminate(result);
            }
            return 1;
        }
        zbuf_reset(result);
        return -1;
	}

    return -1;
}

static int _query(zmap_node_t * node, char *query, zbuf_t *result, int timeout)
{
    zmap_line_t *mnode;
    int ret;

    mnode = (zmap_line_t *)((char *)node + sizeof(zmap_node_t));

    if (zvar_map_pthread_mode) {
        zpthread_lock(&(mnode->locker));
    }
    ret = ___query(node, query, result, timeout);
    if (zvar_map_pthread_mode) {
        zpthread_unlock(&(mnode->locker));
    }

    return ret;
}

zmap_node_t *zmap_node_create_line(char *title, int flags)
{
    zmap_node_t *rnode;
    zmap_line_t *mnode;

    if (zgrid_lookup(zvar_map_node_list, title, (char **)&rnode)) {
        rnode->used++;
        return rnode;
    }

    rnode = (zmap_node_t *)zcalloc(1, sizeof(zmap_node_t) + sizeof(zmap_line_t));
    zgrid_add(zvar_map_node_list, title, rnode, 0);
    rnode->close = _close;
    rnode->query = _query;
    rnode->used = 1;
    rnode->title = zstrdup(title);
    
    mnode = (zmap_line_t *)((char *)rnode + sizeof(zmap_node_t));
    mnode->fd = -1;
    mnode->fp = 0;
    pthread_mutex_init(&(mnode->locker), 0);

    mnode->url = zstrdup(title + 5);

    return rnode;
}
