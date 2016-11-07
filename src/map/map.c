/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-09-04
 * ================================
 */

#include "libzc.h"

int zvar_map_pthread_mode = 1;
zgrid_t *zvar_map_node_list = 0;

pthread_mutex_t locker = PTHREAD_MUTEX_INITIALIZER;

zmap_node_t *zmap_node_create_static(char *title, int flags);
zmap_node_t *zmap_node_create_memcache(char *title, int flags);
zmap_node_t *zmap_node_create_redis(char *title, int flags);
zmap_node_t *zmap_node_create_socket(char *title, int flags);
zmap_node_t *zmap_node_create_flat(char *title, int flags);
zmap_node_t *zmap_node_create_line(char *title, int flags);
zmap_node_t *zmap_node_create_postproxy(char *title, int flags);

static zmap_node_t *zmap_node_create(char *title, int flags)
{
    char type[56];
    char *p;
    int len;

    p = strchr(title, ':');
    if (!p) {
        zfatal("zmap_create: can not create %s", title);
    }
    len = p - title;
    if (len > 50) {
        zfatal("zmap_create: can not create %s", title);
    }
    memcpy(type, title, len);
    type[len] = 0;
    ztolower(type);

    if (!strcmp(type, "static")) {
        return zmap_node_create_static(title, flags);
    } else if (!strcmp(type, "memcache")) {
        return zmap_node_create_memcache(title, flags);
    } else if (!strcmp(type, "redis")) {
        return zmap_node_create_redis(title, flags);
    } else if (!strcmp(type, "socket")) {
        return zmap_node_create_socket(title, flags);
    } else if (!strcmp(type, "flat")) {
        return zmap_node_create_flat(title, flags);
    } else if (!strcmp(type, "line")) {
        return zmap_node_create_line(title, flags);
    } else if (!strcmp(type, "postproxy")) {
        return zmap_node_create_postproxy(title, flags);
    } else {
        zfatal("zmap_create: can not create %s", title);
    }

    return 0;
}

zmap_t *zmap_create(char *map_string, int flags)
{
    zmap_t *rmap;
    zmap_node_t *node;
    zargv_t *args;
    zarray_t *node_list;
    char *p;
    int i;

    rmap = zcalloc(1, sizeof(zmap_t));
    node_list = zarray_create(1);
    args = zargv_create(1);
    zargv_split_append(args, map_string, ",\t ;");

    if (zvar_map_pthread_mode) {
        zpthread_lock(&locker);
    }
    if (!zvar_map_node_list) {
        zvar_map_node_list = zgrid_create();
    }
    ZARGV_WALK_BEGIN(args, p) {
        node = zmap_node_create(p, flags);
        zarray_add(node_list, node);
    }
    ZARGV_WALK_END;
    if (zvar_map_pthread_mode) {
        zpthread_unlock(&locker);
    }

    rmap->node_len = ZARRAY_LEN(node_list);
    rmap->node_list = (zmap_node_t **) zcalloc(rmap->node_len + 1, sizeof(zmap_node_t *));

    i = 0;
    ZARRAY_WALK_BEGIN(node_list, p) {
        rmap->node_list[i++] = (zmap_node_t *) p;
    }
    ZARRAY_WALK_END;

    zargv_free(args);
    zarray_free(node_list, 0, 0);

    return rmap;
}

int zmap_close(zmap_t * zm)
{
    zmap_node_t *node;
    int i;

    if (zm == 0) {
        return 0;
    }

    if (zvar_map_pthread_mode) {
        zpthread_lock(&locker);
    }
    for (i = 0; i < zm->node_len; i++) {
        node = zm->node_list[i];
        node->close(node);
    }
    if (zvar_map_pthread_mode) {
        zpthread_unlock(&locker);
    }
    zfree(zm->node_list);
    zfree(zm);

    return 0;
}

int zmap_query(zmap_t * zm, char *query, zbuf_t * result, int timeout)
{
    zmap_node_t *node;
    int i;
    int ret = 0;
    long t2;
    int left;
    char ch, *p;

    if (zm == 0) {
        return 0;
    }
    p = query;

    while (*p) {
        ch = *p++;
        if ((!isprint(ch)) || isblank(ch)) {
            zbuf_strcpy(result, "zmap_query: query include illegal character");
            return -1;
        }
    }

    zbuf_reset(result);
    if (timeout < 1) {
        timeout = 3600 * 24;
    }
    t2 = ztimeout_set(timeout);

    for (i = 0; i < zm->node_len; i++) {
        node = zm->node_list[i];
        left = ztimeout_left(t2);
        if (left < -1) {
            ret = -2;
            break;
        }
        ret = node->query(node, query, result, left);
        if (ret != 0) {
            break;
        }
    }

    return ret;
}

int zmap_read_line(zstream_t * fp, char *buf, int len, int *reach_end)
{
    if (reach_end) {
        *reach_end = 1;
    }

    len = zfread_line(fp, buf, len);
    if (len < 1) {
        return -1;
    }

    if ((len > 0) && (buf[len - 1] == '\n')) {
        len--;
        if (reach_end) {
            *reach_end = 1;
        }
    }
    if ((len > 0) && (buf[len - 1] == '\r')) {
        len--;
    }
    buf[len] = 0;

    return len;
}

int zmap_title_split(char *opstr, char **list)
{
    char *ps = opstr, *p;
    int count = 0;

    while (count < 9) {
        list[count++] = ps;
        p = strchr(ps, '&');
        if (!p) {
            break;
        }
        *p = 0;
        ps = p + 1;
    }

    return count;
}
