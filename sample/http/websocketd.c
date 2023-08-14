/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2024-05-29
 * ================================
 */

#ifdef __linux__

#include "zc.h"
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>

typedef union
{
    void *ptr;
    struct
    {
        int fd;
        unsigned int sock_type : 8;
        unsigned int is_ssl : 1;
    } sockinfo;
} long_info_t;

typedef void (*cmd_fn_t)(zhttpd_t *httpd);

static void usage()
{
    zprintf("USAGE: %s -listen 0:8899\n", zvar_progname);
    exit(1);
}

static void *do_websocked(void *arg)
{
    zstream_t *fp = (zstream_t *)arg;
    zwebsocketd_t *wsd = zwebsocketd_open(fp);
    char *databuf = (char *)zmalloc(10240 + 1);
    while (1)
    {
        if (zwebsocketd_read_frame_header(wsd) < 1)
        {
            zinfo("zwebsocketd_read_frame_header error(%m)");
            break;
        }
        // 本例子不考虑 fin 为 0 的情况, 不考虑数据太长的情况
        int len = zwebsocketd_read_frame_data(wsd, databuf, 10240);
        if (len < 0)
        {
            zinfo("zwebsocketd_read_frame_data error(%m)");
            break;
        }
        databuf[len] = 0;
        int opcode = zwebsocketd_get_header_opcode(wsd);
        zinfo("read: opcode=%02X, len=%d, %s", opcode, len, (char *)databuf);
        if (opcode == zvar_websocketd_type_ping)
        {
            if (zwebsocketd_send_pong(wsd, databuf, len) < 1)
            {
                zinfo("zwebsocketd_read_frame_data error(%m)");
                break;
            }
            continue;
        }
        if (opcode == zvar_websocketd_type_pong)
        {
            continue;
        }
        if (opcode == zvar_websocketd_type_close)
        {
            zinfo("closed");
            break;
        }
        if (opcode == zvar_websocketd_type_text)
        {
            if (zwebsocketd_send_text(wsd, "your:", 5) < 1)
            {
                zinfo("zwebsocketd_read_frame_data error(%m)");
                break;
            }
            if (zwebsocketd_send_text(wsd, databuf, len) < 1)
            {
                zinfo("zwebsocketd_read_frame_data error(%m)");
                break;
            }
            continue;
        }
        if (opcode == zvar_websocketd_type_binary) {
            if (zwebsocketd_send_text(wsd, "binary", 6) < 1)
            {
                zinfo("zwebsocketd_read_frame_data error(%m)");
                break;
            }
            continue;
        }
        zinfo("unknown opcode: %02X", opcode);
        break;
    }
    zfree(databuf);
    zwebsocketd_close(wsd, 1);
    return 0;
}

static void httpd_handler(zhttpd_t *httpd)
{
    // const char *path = zhttpd_request_get_path(httpd);
    // if (path == '/opt/somepath/') { }
    // else { }
    if (!zhttpd_is_websocket(httpd))
    {
        zhttpd_response_file(httpd, "resource_httpd/websocketd.html", 0, 0);
        return;
    }
    // 如果是 websocket 请求
    if (zhttpd_websocket_shakehand(httpd) < 1)
    {
        zhttpd_set_stop(httpd);
        return;
    }

    zhttpd_set_stop(httpd);
    zstream_t *fp = zhttpd_detach_stream(httpd);
    zcoroutine_go(do_websocked, fp, 128);
    return;
}

static void *do_httpd(void *arg)
{
    long_info_t linfo;
    linfo.ptr = arg;

    int fd = linfo.sockinfo.fd;

    zhttpd_t *httpd;
    httpd = zhttpd_open_fd(fd);

    while (1)
    {
        if (zhttpd_request_read_all(httpd) < 1)
        {
            break;
        }
        httpd_handler(httpd);
        zhttpd_response_flush(httpd);
        if (!zhttpd_maybe_continue(httpd))
        {
            break;
        }
    }
    zhttpd_close(httpd, 1);

    return 0;
}

static void *accept_incoming(void *arg)
{
    long_info_t linfo;
    linfo.ptr = arg;
    int sock = linfo.sockinfo.fd;
    int sock_type = linfo.sockinfo.sock_type;

    while (1)
    {
        ztimed_read_wait(sock, 10);
        int fd = zaccept(sock, sock_type);
        if (fd < 0)
        {
            if (errno == EAGAIN)
            {
                continue;
            }
            if (errno == EINTR)
            {
                continue;
            }
            zfatal("accept: %m");
        }
        linfo.sockinfo.fd = fd;
        zcoroutine_go(do_httpd, linfo.ptr, -1);
    }
    return arg;
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    zvar_httpd_no_cache = 1;
    zmain_argument_run(argc, argv);

    char *listen = zconfig_get_str(zvar_default_config, "listen", "");
    if (zempty(listen))
    {
        usage();
    }

    zcoroutine_base_init();

    long_info_t linfo;
    int fd, type;
    fd = zlisten(listen, &type, 5);
    if (fd < 0)
    {
        zprintf("ERROR can not open %s(%m)\n", listen);
        exit(1);
    }
    linfo.sockinfo.fd = fd;
    linfo.sockinfo.sock_type = type;

    zcoroutine_go(accept_incoming, linfo.ptr, -1);

    zcoroutine_base_run();

    zclose(fd);

    zcoroutine_base_fini();

    zinfo("EXIT");

    return 0;
}

#else // __linux__
int main()
{
    return 0;
}
#endif // __linux__
