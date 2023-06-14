/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-03-15
 * ================================
 */

#include "zc.h"

#define proxy_set_errmsg(fmt, args...) \
    zbuf_reset(client->error_msg); zbuf_printf_1024(client->error_msg, fmt, ##args);

struct zsqlite3_proxy_client_t {
    char *destination;
    zstream_t *fp;
    zbuf_t *error_msg;
    zbuf_t **row;
    short int column_count;
    unsigned short int auto_reconnect:1;
    unsigned short int query_over:1;
    unsigned short int error_log_flag:1;
};

zsqlite3_proxy_client_t *zsqlite3_proxy_client_connect(const char *destination)
{
    int fd = zconnect(destination, 0);
    if (fd < 0) {
        return 0;
    }
    znonblocking(fd, 1);
    zsqlite3_proxy_client_t *client = (zsqlite3_proxy_client_t *)zcalloc(1, sizeof(zsqlite3_proxy_client_t));
    client->destination = zstrdup(destination);
    client->auto_reconnect = 0;
    client->fp = zstream_open_fd(fd);
    client->error_msg = zbuf_create(128);
    client->column_count = 0;
    client->row = 0;
    client->query_over = 1;

    return client;
}

void zsqlite3_proxy_client_close(zsqlite3_proxy_client_t *client)
{
    zfree(client->destination);
    if (client->fp) {
        zstream_close(client->fp, 1);
    }
    zbuf_free(client->error_msg);
    if (client->row) {
        for (int i = 0; i < client->column_count; i++) {
            zbuf_free(client->row[i]);
        }
        zfree(client->row);
    }
    zfree(client);
}

void zsqlite3_proxy_client_set_auto_reconnect(zsqlite3_proxy_client_t *client, zbool_t auto_reconnect)
{
    client->auto_reconnect = auto_reconnect;
}

static zbool_t _zsqlite3_proxy_client_connect(zsqlite3_proxy_client_t *client, zbool_t force)
{
    if (client->fp) {
        return 1;
    }
    if ((force == 0) && (client->auto_reconnect == 0)) {
        return 0;
    }
    for (int retry = 0; retry < 2; retry++) {
        int fd = zconnect(client->destination, 0);
        if (fd < 0) {
            continue;
        }
        znonblocking(fd, 1);
        client->fp = zstream_open_fd(fd);
        break;
    }
    if (client->fp) {
        return 1;
    }
    return 0;
}

static void _zsqlite3_proxy_client_disconnect(zsqlite3_proxy_client_t *client)
{
    if (!client->fp) {
        return;
    }
    zstream_close(client->fp, 1);
    client->fp = 0;
    client->query_over = 1;
}

static void _zsqlite3_proxy_client_clear_query(zsqlite3_proxy_client_t *client)
{
    if (client->query_over) {
        return;
    }
    if (!client->fp) {
        client->query_over = 1;
        return;
    }

    client->query_over = 1;
    zbuf_t **row;
    while(1) {
        int ret = zsqlite3_proxy_client_get_row(client, &row);
        if (ret == 0) {
            break;
        }
        if (ret < 0) {
            _zsqlite3_proxy_client_disconnect(client);
            _zsqlite3_proxy_client_connect(client, 0);
            return;
        }
    }
    return;
}

int zsqlite3_proxy_client_log(zsqlite3_proxy_client_t *client, const char *sql, int len)
{
    _zsqlite3_proxy_client_clear_query(client);

    if (zempty(sql) || (len == 0)) {
        return 1;
    }
    if (len < 0) {
        len = strlen(sql);
    }

    if (_zsqlite3_proxy_client_connect(client, 0) == 0) {
        return -1;
    }

    zstream_write_cint(client->fp, len+1);
    zstream_putc(client->fp, 'L');
    zstream_write(client->fp, sql, len);
    zstream_flush(client->fp);
    if (zstream_is_exception(client->fp)) {
        proxy_set_errmsg("write");
        _zsqlite3_proxy_client_disconnect(client);
        if (client->error_log_flag) {
            zinfo("ERROR zsqlite3_proxy_client_log: write");
        }
        return -1;
    }
    return 1;
}

int zsqlite3_proxy_client_exec(zsqlite3_proxy_client_t *client, const char *sql, int len)
{
    _zsqlite3_proxy_client_clear_query(client);

    if (zempty(sql) || (len == 0)) {
        return 1;
    }
    if (len < 0) {
        len = strlen(sql);
    }

    if (_zsqlite3_proxy_client_connect(client, 0) == 0) {
        return -1;
    }

    zstream_write_cint(client->fp, len+1);
    zstream_putc(client->fp, 'E');
    zstream_write(client->fp, sql, len);
    zstream_flush(client->fp);
    if (zstream_is_exception(client->fp)) {
        proxy_set_errmsg("write");
        _zsqlite3_proxy_client_disconnect(client);
        if (client->error_log_flag) {
            zinfo("ERROR zsqlite3_proxy_client_exec: write");
        }
        return -1;
    }

    len = zstream_get_cint(client->fp);
    if ((len < 1) || (len > 10000)) {
        proxy_set_errmsg("read");
        _zsqlite3_proxy_client_disconnect(client);
        if (client->error_log_flag) {
            zinfo("ERROR zsqlite3_proxy_client_exec: read");
        }
        return -1;
    }
    int r = zstream_getc(client->fp);
    if (r == 'O') {
        if (len > 1) {
            zstream_readn(client->fp, 0, len-1);
        }
        if (zstream_is_exception(client->fp)) {
            _zsqlite3_proxy_client_disconnect(client);
            _zsqlite3_proxy_client_connect(client, 0);
        }
        return 1;
    } else if (r < 0) {
        _zsqlite3_proxy_client_disconnect(client);
        proxy_set_errmsg("read");
        if (client->error_log_flag) {
            zinfo("ERROR zsqlite3_proxy_client_exec: read");
        }
        return -1;
    } else {
        zstream_readn(client->fp, client->error_msg, len-1);
        if (zstream_is_exception(client->fp)) {
            _zsqlite3_proxy_client_disconnect(client);
        }
        if (client->error_log_flag) {
            zinfo("ERROR zsqlite3_proxy_client_exec: server return: %s", zbuf_data(client->error_msg));
        }
        return -1;
    }
}

int zsqlite3_proxy_client_query(zsqlite3_proxy_client_t *client, const char *sql, int len)
{
    _zsqlite3_proxy_client_clear_query(client);
    if (client->row) {
        for (int i = 0; i < client->column_count; i++) {
            zbuf_free(client->row[i]);
        }
        zfree(client->row);
        client->row = 0;
    }

    if (zempty(sql) || (len == 0)) {
        return 1;
    }
    if (len < 0) {
        len = strlen(sql);
    }

    if (_zsqlite3_proxy_client_connect(client, 0) == 0) {
        return -1;
    }

    client->query_over = 0;

    zstream_write_cint(client->fp, len+1);
    zstream_putc(client->fp, 'Q');
    zstream_write(client->fp, sql, len);
    zstream_flush(client->fp);
    if (zstream_is_exception(client->fp)) {
        proxy_set_errmsg("write");
        _zsqlite3_proxy_client_disconnect(client);
        if (client->error_log_flag) {
            zinfo("ERROR zsqlite3_proxy_client_query: write");
        }
        return -1;
    }

    len = zstream_get_cint(client->fp);
    if ((len < 1) || (len > 10000)) {
        proxy_set_errmsg("read");
        _zsqlite3_proxy_client_disconnect(client);
        return -1;
    }
    int r = zstream_getc(client->fp);
    if (r == 'E') {
        if (len > 1) {
            zstream_readn(client->fp, client->error_msg, len-1);
            if (client->error_log_flag) {
                zinfo("ERROR zsqlite3_proxy_client_query: server return: %s", zbuf_data(client->error_msg));
            }
        } else {
            proxy_set_errmsg("unknown error");
            zinfo("ERROR zsqlite3_proxy_client_query: unknown server error");
        }
        if (zstream_is_exception(client->fp)) {
            _zsqlite3_proxy_client_disconnect(client);
            _zsqlite3_proxy_client_connect(client, 0);
        }
        return -1;
    } else if (r < 0) {
        _zsqlite3_proxy_client_disconnect(client);
        proxy_set_errmsg("read");
        if (client->error_log_flag) {
            zinfo("ERROR zsqlite3_proxy_client_query: read");
        }
        return -1;
    } else {
        zstream_readn(client->fp, client->error_msg, len-1);
        if (zstream_is_exception(client->fp)) {
            _zsqlite3_proxy_client_disconnect(client);
            if (client->error_log_flag) {
                zinfo("ERROR zsqlite3_proxy_client_query: server return: %s", zbuf_data(client->error_msg));
            }
            return -1;
        }
        client->column_count = atoi(zbuf_data(client->error_msg));
        return 1;
    }
}

int zsqlite3_proxy_client_get_row(zsqlite3_proxy_client_t *client, zbuf_t ***row)
{
    if (client->row) {
        for (int i = 0; i < client->column_count; i++) {
            zbuf_free(client->row[i]);
        }
        zfree(client->row);
        client->row = 0;
    }
    if (client->fp == 0) {
        proxy_set_errmsg("closed");
        client->query_over = 1;
        return -1;
    }
    int len = zstream_get_cint(client->fp);
    if (len < 1) {
        _zsqlite3_proxy_client_disconnect(client);
        proxy_set_errmsg("read");
        if (client->error_log_flag) {
            zinfo("ERROR zsqlite3_proxy_client_get_row: read");
        }
        return -1;
    }
    int r = zstream_getc(client->fp);
    if (r<0) {
        _zsqlite3_proxy_client_disconnect(client);
        proxy_set_errmsg("read");
        if (client->error_log_flag) {
            zinfo("ERROR zsqlite3_proxy_client_get_row: read");
        }
        return -1;
    }
    if (r == 'E') {
        if (len > 1) {
            zstream_readn(client->fp, client->error_msg, len-1);
            if (client->error_log_flag) {
                zinfo("ERROR zsqlite3_proxy_client_get_row: server return: %s", zbuf_data(client->error_msg));
            }
        } else {
            proxy_set_errmsg("unknown error");
            if (client->error_log_flag) {
                zinfo("ERROR zsqlite3_proxy_client_get_row: unknown server error");
            }
        }
        if (zstream_is_exception(client->fp)) {
            _zsqlite3_proxy_client_disconnect(client);
            _zsqlite3_proxy_client_connect(client, 0);
            return -1;
        }
        return -1;
    }
    if (r == 'O') {
        client->query_over = 1;
        if (len > 1) {
            zstream_readn(client->fp, client->error_msg, len-1);
            if (client->error_log_flag) {
                zinfo("ERROR zsqlite3_proxy_client_get_row: server return: %s", zbuf_data(client->error_msg));
            }
        }
        if (zstream_is_exception(client->fp)) {
            _zsqlite3_proxy_client_disconnect(client);
            _zsqlite3_proxy_client_connect(client, 0);
            return 0;
        }
        return 0;
    }

    if (r != '*') {
        _zsqlite3_proxy_client_disconnect(client);
        proxy_set_errmsg("read star");
        if (client->error_log_flag) {
            zinfo("ERROR zsqlite3_proxy_client_get_row: read");
        }
        return -1;
    }

    zbuf_t *cbuf = zbuf_create(len);
    zstream_readn(client->fp, cbuf, len-1);
    if (zstream_is_exception(client->fp)) {
        _zsqlite3_proxy_client_disconnect(client);
        zbuf_free(cbuf);
        return -1;
    }
    client->row = (zbuf_t **)zcalloc(client->column_count, sizeof(zbuf_t *));
    *row = client->row;

    char *ps = zbuf_data(cbuf);
    len = zbuf_len(cbuf);
    int c = 0;
    while(len > 0) {
        char *result_data;
        int result_len;
        int offset = zcint_data_unescape(ps, len, (void **)&result_data, &result_len);
        if ((offset < 0) || (offset > len)) {
            zbuf_free(cbuf);
            _zsqlite3_proxy_client_disconnect(client);
            proxy_set_errmsg("unescape row");
            if (client->error_log_flag) {
                zinfo("ERROR zsqlite3_proxy_client_get_row: unescape row");
            }
            return -1;
        }

        client->row[c] = zbuf_create(result_len);
        zbuf_memcpy(client->row[c], result_data, result_len);
        c++;
        len -= offset;
        ps += offset;
        if (c == client->column_count) {
            break;
        }
    }

    zbuf_free(cbuf);
    return 1;
}

int zsqlite3_proxy_client_get_column(zsqlite3_proxy_client_t *client)
{
    return client->column_count;
}

const char *zsqlite3_proxy_client_get_error_msg(zsqlite3_proxy_client_t *client)
{
    return zbuf_data(client->error_msg);
}

void zsqlite3_proxy_client_set_error_log(zsqlite3_proxy_client_t *client, int flag)
{
    client->error_log_flag = (flag?1:0);
}

int zsqlite3_proxy_client_quick_fetch_one_row(zsqlite3_proxy_client_t *db, const char *sql, int len, zbuf_t ***result_row)
{
    if (len < 0) {
        len = strlen(sql);
    }

    if (zsqlite3_proxy_client_query(db, sql, len) < 0) {
        return -1;
    }

    zbuf_t **row;
    int exists = 0;
    while (1) {
        int r = zsqlite3_proxy_client_get_row(db, &row);
        if (r < 0) {
            return -1;
        }
        if (r == 0) {
            break;
        }
        exists = 1;
        if (result_row) {
            *result_row = row;
        }
        break;
    }
    return exists;
}

