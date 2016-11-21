/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-09-06
 * ================================
 */

#include "libzc.h"

typedef struct zfinder_postproxy_t zfinder_postproxy_t;
struct zfinder_postproxy_t {
    char *postfix_dict;
    int fd;
    zstream_t *fp;
};

static int _close(zfinder_t * finder)
{
    zfinder_postproxy_t *my_db;

    my_db = (zfinder_postproxy_t *) (finder->db);

    if (my_db->fp) {
        zstream_close_FD(my_db->fp);
    }
    if (my_db->fd != -1) {
        close(my_db->fd);
    }

    zfree(my_db);

    return 0;
}

static int ___close(zfinder_t * finder)
{
    zfinder_postproxy_t *my_db;

    my_db = (zfinder_postproxy_t *) (finder->db);
    if (my_db->fp) {
        zstream_close_FD(my_db->fp);
        my_db->fp = 0;
    }
    if (my_db->fd != -1) {
        close(my_db->fd);
        my_db->fd = -1;
    }

    return 0;
}

static int ___connect(zfinder_t * finder, int timeout)
{
    zfinder_postproxy_t *my_db;

    my_db = (zfinder_postproxy_t *) (finder->db);
    if (my_db->fp) {
        return 0;
    }

    if (my_db->fd < 0) {
        my_db->fd = zconnect(finder->uri, timeout);
    }
    if (my_db->fd < 0) {
        return -1;
    }
    my_db->fp = zstream_open_FD(my_db->fd);

    return 0;
}

static int ___get(zfinder_t * finder, const char *query, zbuf_t * result, int timeout)
{
    int i, ret;
    long dtime = ztimeout_set(timeout);
    char buf[102400 + 10];
    ZSTACK_BUF(zb, 10240);
    zfinder_postproxy_t *my_db;
    int status;

    my_db = (zfinder_postproxy_t *) (finder->db);

    for (i = 0; i < 2; i++) {
        zbuf_reset(result);
        if (i) {
            ___close(finder);
        }
        ret = ___connect(finder, ztimeout_left(dtime));
        if (ret < 0) {
            zbuf_printf_1024(result, "zfinder_get: %s : connection error", finder->title);
            return ret;
        }
        zstream_set_timeout(my_db->fp, ztimeout_left(dtime));

        zbuf_strcpy(zb, "request");
        zbuf_putchar(zb, '\0');
        zbuf_strcat(zb, "lookup");
        zbuf_putchar(zb, '\0');

        zbuf_strcat(zb, "table");
        zbuf_putchar(zb, '\0');
        zbuf_strcat(zb, my_db->postfix_dict);
        zbuf_putchar(zb, '\0');

        zbuf_strcat(zb, "flags");
        zbuf_putchar(zb, '\0');
        sprintf(buf, "%d", (1 << 6));
        zbuf_strcat(zb, buf);
        zbuf_putchar(zb, '\0');

        zbuf_strcat(zb, "key");
        zbuf_putchar(zb, '\0');
        zbuf_strcat(zb, query);
        zbuf_putchar(zb, '\0');

        zbuf_putchar(zb, '\0');

        if ((zstream_write_n(my_db->fp, ZBUF_DATA(zb), ZBUF_LEN(zb)) < 0) || (ZSTREAM_FLUSH(my_db->fp) < 0)) {
            zbuf_printf_1024(result, "zfinder_get: %s : write error", finder->title);
            continue;
        }

        ret = zstream_read_delimiter(my_db->fp, buf, 102400, '\0');
        if ((ret != 7) || (strcmp(buf, "status"))) {
            zbuf_printf_1024(result, "zfinder_get: %s : read error, need status name", finder->title);
            continue;
        }
        ret = zstream_read_delimiter(my_db->fp, buf, 102400, '\0');
        if (ret != 2) {
            zbuf_printf_1024(result, "zfinder_get: %s : read error, need status value", finder->title);
            continue;
        }
        status = atoi(buf);

        ret = zstream_read_delimiter(my_db->fp, buf, 102400, '\0');
        if ((ret != 6) || (strcmp(buf, "value"))) {
            zbuf_printf_1024(result, "zfinder_get: %s : read error, need value name", finder->title);
            continue;
        }
        ret = zstream_read_delimiter(my_db->fp, buf, 102400, '\0');
        if (ret < 0) {
            zbuf_printf_1024(result, "zfinder_get: %s : read error, need value value", finder->title);
            continue;
        }
        if (zstream_read_delimiter(my_db->fp, buf + ret + 10, 10, '\0') != 1) {
            zbuf_printf_1024(result, "zfinder_get: %s : read error, need end", finder->title);
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

        zbuf_printf_1024(result, "zfinder_get: %s : read error, postproxy return: %d,", finder->title, status);
        if (ret > 1) {
            zbuf_memcat(result, buf, ret - 1);
        }
        zbuf_terminate(result);

        return -1;
    }

    return -1;
}

static int _get(zfinder_t * finder, const char *query, zbuf_t * result, int timeout)
{
    int ret;

    ret = ___get(finder, query, result, timeout);

    return ret;
}

int zfinder_create_postproxy(zfinder_t *finder)
{
    zfinder_postproxy_t *my_db;

    my_db = (zfinder_postproxy_t *)zcalloc(1, sizeof(zfinder_postproxy_t));
    finder->db = my_db;
    finder->close = _close;
    finder->get = _get;

    my_db->fd = -1;
    my_db->fp = 0;
    if (!zdict_lookup(finder->parameters, "dictname", &(my_db->postfix_dict))){
        zfree(my_db);
        finder->db = 0;
        return -1;
    } 

    return 0;
}
