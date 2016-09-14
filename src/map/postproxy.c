/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-09-06
 * ================================
 */

#include "libzc.h"

typedef struct zmap_postproxy_t zmap_postproxy_t;
struct zmap_postproxy_t {
    char *url;
    char *postfix_dict;
    pthread_mutex_t locker;
    int fd;
    zstream_t *fp;
};

static int _close(zmap_node_t *node)
{
    zmap_postproxy_t *mnode;

    node->used --;
    if (node->used > 0) {
        return 0;
    }
    
    zgrid_delete(zvar_map_node_list, node->title, 0);

    mnode = (zmap_postproxy_t *)((char *)node + sizeof(zmap_node_t));

    if(mnode->fp) {
        zfclose_FD(mnode->fp);
    }
    if(mnode->fd != -1) {
        close(mnode->fd);
    }
    zfree(mnode->url);
    zfree(mnode->postfix_dict);

    zfree(node);

    return 0;
}

static int ___close(zmap_node_t *node)
{
    zmap_postproxy_t *mnode;
   
    mnode = (zmap_postproxy_t *)((char *)node + sizeof(zmap_node_t));
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
    zmap_postproxy_t *mnode;
   
    mnode = (zmap_postproxy_t *)((char *)node + sizeof(zmap_node_t));
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
    int i, ret;
    long dtime = ztimeout_set(timeout);
    char buf[102400 + 10];
    ZSTACK_BUF(zb, 10240);
    zmap_postproxy_t *mnode;
    int status;
   
    mnode = (zmap_postproxy_t *)((char *)node + sizeof(zmap_node_t));

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
        
        zbuf_strcpy(zb, "request");
        zbuf_putchar(zb, '\0');
        zbuf_strcat(zb, "lookup");
        zbuf_putchar(zb, '\0');

        zbuf_strcat(zb, "table");
        zbuf_putchar(zb, '\0');
        zbuf_strcat(zb, mnode->postfix_dict);
        zbuf_putchar(zb, '\0');

        zbuf_strcat(zb, "flags");
        zbuf_putchar(zb, '\0');
        sprintf(buf, "%d", (1<<6));
        zbuf_strcat(zb, buf);
        zbuf_putchar(zb, '\0');

        zbuf_strcat(zb, "key");
        zbuf_putchar(zb, '\0');
        zbuf_strcat(zb, query);
        zbuf_putchar(zb, '\0');

        zbuf_putchar(zb, '\0');

        if ((zfwrite_n(mnode->fp, ZBUF_DATA(zb), ZBUF_LEN(zb)) < 0) || (ZFFLUSH(mnode->fp) < 0)) {
            zbuf_sprintf(result, "zmap_query: %s : write error", node->title);
            continue;
        }

        ret = zfread_delimiter(mnode->fp, buf, 102400, '\0');
        if ((ret != 7) || (strcmp(buf, "status"))) {
            zbuf_sprintf(result, "zmap_query: %s : read error, need status name", node->title);
			continue;
        }
        ret = zfread_delimiter(mnode->fp, buf, 102400, '\0');
        if (ret != 2) {
            zbuf_sprintf(result, "zmap_query: %s : read error, need status value", node->title);
			continue;
        }
        status = atoi(buf);

        ret = zfread_delimiter(mnode->fp, buf, 102400, '\0');
        if ((ret != 6) || (strcmp(buf, "value"))) {
            zbuf_sprintf(result, "zmap_query: %s : read error, need value name", node->title);
			continue;
        }
        ret = zfread_delimiter(mnode->fp, buf, 102400, '\0');
        if (ret < 0) {
            zbuf_sprintf(result, "zmap_query: %s : read error, need value value", node->title);
            continue;
        }
        if (zfread_delimiter(mnode->fp, buf+ret+10, 10, '\0') != 1) {
            zbuf_sprintf(result, "zmap_query: %s : read error, need end", node->title);
            continue;
        }
        zbuf_reset(result);
        if (status == 0) {
            if (ret > 1) {
                zbuf_memcpy(result, buf, ret - 1);
            }
            zbuf_terminate(result);
            return 1;
        }
        if (status == 1) {
            return 0;
        }

        zbuf_sprintf(result, "zmap_query: %s : read error, proxy return: %d,", node->title, status);
        if (ret > 1) {
            zbuf_memcat(result, buf, ret - 1);
        }
        zbuf_terminate(result);

        return -1;
	}

    return -1;
}

static int _query(zmap_node_t * node, char *query, zbuf_t *result, int timeout)
{
    zmap_postproxy_t *mnode;
    int ret;

    mnode = (zmap_postproxy_t *)((char *)node + sizeof(zmap_node_t));

    if (zvar_map_pthread_mode) {
        zpthread_lock(&(mnode->locker));
    }
    ret = ___query(node, query, result, timeout);
    if (zvar_map_pthread_mode) {
        zpthread_unlock(&(mnode->locker));
    }

    return ret;
}

zmap_node_t *zmap_node_create_postproxy(char *title, int flags)
{
    zmap_node_t *rnode;
    zmap_postproxy_t *mnode;
    char *args[10] = {"", "", ""};
    char opstr[1024];

    if (zgrid_lookup(zvar_map_node_list, title, (char **)&rnode)) {
        rnode->used++;
        return rnode;
    }

    rnode = (zmap_node_t *)zcalloc(1, sizeof(zmap_node_t) + sizeof(zmap_postproxy_t));
    zgrid_add(zvar_map_node_list, title, rnode, 0);
    rnode->close = _close;
    rnode->query = _query;
    rnode->used = 1;
    rnode->title = zstrdup(title);
    
    mnode = (zmap_postproxy_t *)((char *)rnode + sizeof(zmap_node_t));
    mnode->fd = -1;
    mnode->fp = 0;
    pthread_mutex_init(&(mnode->locker), 0);

    zstrncpy(opstr, title + 10, 1000);
    zmap_title_split(opstr, args);
    if (args[0][0] == 0) {
        zfatal("zmap_create: %s", title);
    }
    mnode->url = zstrdup(args[0]);
    mnode->postfix_dict = zstrdup(args[1]);

    return rnode;
}
