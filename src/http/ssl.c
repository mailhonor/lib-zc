/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-04-05
 * ================================
 */

#include "httpd.h"

zhttpd_t *zhttpd_open_ssl(SSL *ssl)
{
    zhttpd_t * httpd = _zhttpd_malloc_struct_general();
    httpd->fp = zstream_open_ssl(ssl);
    zstream_set_read_wait_timeout(httpd->fp, httpd->read_wait_timeout);
    zstream_set_write_wait_timeout(httpd->fp, httpd->write_wait_timeout);
    httpd->ssl_mode = 1;
    return httpd;
}
