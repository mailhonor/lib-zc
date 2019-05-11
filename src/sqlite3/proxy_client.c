/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
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
    short int column_count:14;
    unsigned short int auto_reconnect:1;
    unsigned short int query_over:1;
};

zsqlite3_proxy_client_t *zsqlite3_proxy_client_connect(const char *destination, zbool_t auto_reconnect)
{
    int fd = zconnect(destination, 0, 0);
    if (fd < 0) {
        return 0;
    }
    zsqlite3_proxy_client_t *client = (zsqlite3_proxy_client_t *)zcalloc(1, sizeof(zsqlite3_proxy_client_t));
    client->destination = zstrdup(destination);
    client->auto_reconnect = auto_reconnect;
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

static zbool_t _zsqlite3_proxy_client_connect(zsqlite3_proxy_client_t *client, zbool_t force)
{
    if (client->fp) {
        return 1;
    }
    if ((force == 0) && (client->auto_reconnect == 0)) {
        return 0;
    }
    for (int retry = 0; retry < 2; retry++) {
        int fd = zconnect(client->destination, 0, 0);
        if (fd < 0) {
            continue;
        }
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

    zstream_write_size_data_size(client->fp, len+1);
    zstream_putc(client->fp, 'L');
    zstream_write(client->fp, sql, len);
    zstream_flush(client->fp);
    if (zstream_is_exception(client->fp)) {
        proxy_set_errmsg("write");
        _zsqlite3_proxy_client_disconnect(client);
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

    zstream_write_size_data_size(client->fp, len+1);
    zstream_putc(client->fp, 'E');
    zstream_write(client->fp, sql, len);
    zstream_flush(client->fp);
    if (zstream_is_exception(client->fp)) {
        proxy_set_errmsg("write");
        _zsqlite3_proxy_client_disconnect(client);
        return -1;
    }

    len = zstream_size_data_get_size(client->fp);
    if ((len < 1) || (len > 10000)) {
        proxy_set_errmsg("read");
        _zsqlite3_proxy_client_disconnect(client);
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
        return -1;
    } else {
        zstream_readn(client->fp, client->error_msg, len-1);
        if (zstream_is_exception(client->fp)) {
            _zsqlite3_proxy_client_disconnect(client);
        }
        return -1;
    }
}

int zsqlite3_proxy_client_query(zsqlite3_proxy_client_t *client, const char *sql, int len)
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

    client->query_over = 0;

    zstream_write_size_data_size(client->fp, len+1);
    zstream_putc(client->fp, 'Q');
    zstream_write(client->fp, sql, len);
    zstream_flush(client->fp);
    if (zstream_is_exception(client->fp)) {
        proxy_set_errmsg("write");
        _zsqlite3_proxy_client_disconnect(client);
        return -1;
    }

    len = zstream_size_data_get_size(client->fp);
    if ((len < 1) || (len > 10000)) {
        proxy_set_errmsg("read");
        _zsqlite3_proxy_client_disconnect(client);
        return -1;
    }
    int r = zstream_getc(client->fp);
    if (r == 'E') {
        if (len > 1) {
            zstream_readn(client->fp, client->error_msg, len-1);
        } else {
            proxy_set_errmsg("unknown error");
        }
        if (zstream_is_exception(client->fp)) {
            _zsqlite3_proxy_client_disconnect(client);
            _zsqlite3_proxy_client_connect(client, 0);
        }
        return -1;
    } else if (r < 0) {
        _zsqlite3_proxy_client_disconnect(client);
        proxy_set_errmsg("read");
        return -1;
    } else {
        zstream_readn(client->fp, client->error_msg, len-1);
        if (zstream_is_exception(client->fp)) {
            _zsqlite3_proxy_client_disconnect(client);
            return -1;
        }
        client->column_count = atoi(zbuf_data(client->error_msg));
        return 1;
    }
}

int zsqlite3_proxy_client_get_row(zsqlite3_proxy_client_t *client, zbuf_t ***row)
{
    if (client->fp == 0) {
        proxy_set_errmsg("closed");
        client->query_over = 1;
        return -1;
    }
    int len = zstream_size_data_get_size(client->fp);
    if (len < 1) {
        proxy_set_errmsg("read");
        _zsqlite3_proxy_client_disconnect(client);
        return -1;
    }
    int r = zstream_getc(client->fp);
    if (r<0) {
        proxy_set_errmsg("read");
        _zsqlite3_proxy_client_disconnect(client);
        return -1;
    }
    if (r == 'E') {
        if (len > 1) {
            zstream_readn(client->fp, client->error_msg, len-1);
        } else {
            proxy_set_errmsg("unknown error");
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
        }
        if (zstream_is_exception(client->fp)) {
            _zsqlite3_proxy_client_disconnect(client);
            _zsqlite3_proxy_client_connect(client, 0);
            return 0;
        }
        return 0;
    }

    if (r != '*') {
        proxy_set_errmsg("read star");
        _zsqlite3_proxy_client_disconnect(client);
        return -1;
    }

    zbuf_t *cbuf = zbuf_create(len);
    zstream_readn(client->fp, cbuf, len-1);
    if (zstream_is_exception(client->fp)) {
        _zsqlite3_proxy_client_disconnect(client);
        zbuf_free(cbuf);
        return -1;
    }
    if (client->row) {
        for (int i = 0; i < client->column_count; i++) {
            zbuf_free(client->row[i]);
        }
        zfree(client->row);
    }
    client->row = (zbuf_t **)zcalloc(client->column_count, sizeof(zbuf_t *));
    *row = client->row;

    char *ps = zbuf_data(cbuf);
    len = zbuf_len(cbuf);
    int c = 0;
    while(len > 0) {
        char *result_data;
        int result_len;
        int offset = zsize_data_unescape(ps, len, (void **)&result_data, &result_len);
        if ((offset < 0) || (offset > len)) {
            proxy_set_errmsg("unescape row");
            _zsqlite3_proxy_client_disconnect(client);
            zbuf_free(cbuf);
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
