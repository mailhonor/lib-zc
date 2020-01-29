/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2019-06-26
 * ================================
 */

#include "zc.h"
#include <errno.h>

static char *server_address=0;
static void ___usage()
{
    printf("USAGE: %s -server address\n", zvar_progname);
    exit(1);
}


static zstream_t *do_prepare_and_welcome(void *context)
{
    int fd = (int)(long)context;
    znonblocking(fd, 1);
    zstream_t *fp = zstream_open_fd(fd);
    zstream_puts(fp, "220 blackhole ESMTP\r\n");
    return fp;
}

typedef void (*smtpd_cmd_t) (zstream_t *, zbuf_t *tmpbf, int *over);

static void cmd_no_such_cmd(zstream_t *fp, zbuf_t *tmpbf, int *over)
{
    zstream_puts(fp, "502 Error: command not recognized\r\n");
}
static void cmd_ehlo(zstream_t *fp, zbuf_t *tmpbf, int *over)
{
    zstream_puts(fp, "250-PIPELINING\r\n250-AUTH PLAIN\r\n250-8BITMIME\r\n250 OK\r\n");
}

static void cmd_250(zstream_t *fp, zbuf_t *tmpbf, int *over)
{
    zstream_puts(fp, "250 OK\r\n");
}

static void cmd_auth(zstream_t *fp, zbuf_t *tmpbf, int *over)
{
    zstream_puts(fp, "334 dXNlcm5hbWU6\r\n");
    zstream_gets(fp, tmpbf, 10240);
    zstream_puts(fp, "334 UGFzc3dvcmQ6\r\n");
    zstream_gets(fp, tmpbf, 10240);
    zstream_puts(fp, "235 Authentication successful\r\n");
}

static void cmd_data(zstream_t *fp, zbuf_t *tmpbf, int *over)
{
    zstream_puts(fp, "250 OK\r\n");
}

static void cmd_quit(zstream_t *fp, zbuf_t *tmpbf, int *over)
{
    zstream_puts(fp, "250 OK\r\n");
    if (over) {
        *over = 1;
    }
}

static smtpd_cmd_t get_cmd(zstream_t *fp, zbuf_t *bf)
{
    if (zstream_gets(fp, bf, 10240) < 1) {
        return 0;
    }
    const char *cmdp = zbuf_data(bf);
    if (!strncasecmp(cmdp, "ehlo", 4)) {
        return cmd_ehlo;
    }
    if (!strncasecmp(cmdp, "helo", 4)) {
        return cmd_250;
    }
    if (!strncasecmp(cmdp, "mail", 4)) {
        return cmd_250;
    }
    if (!strncasecmp(cmdp, "rcpt", 4)) {
        return cmd_250;
    }
    if (!strncasecmp(cmdp, "auth", 4)) {
        return cmd_auth;
    }
    if (!strncasecmp(cmdp, "data", 4)) {
        return cmd_data;
    }
    if (!strncasecmp(cmdp, "quit", 4)) {
        return cmd_quit;
    }
    return cmd_no_such_cmd;
}

static void *do_smtpd(void *context)
{
    zstream_t *fp = do_prepare_and_welcome(context);
    zbuf_t *bf = zbuf_create(1024);

    while(1) {
        int over = 0;
        smtpd_cmd_t cmd = get_cmd(fp, bf);
        if (!cmd) {
            break;
        }
        cmd(fp, bf, &over);
        zstream_flush(fp);
        if (over == 1) {
            break;
        }
    }

    zstream_close(fp, 1);
    zbuf_free(bf);
    return context;
}

void *do_listen(void *context)
{
    int sock_type;
    int sock = zlisten(server_address, &sock_type, 5); 
    if (sock < 0) {
        zfatal("listen: %s (%m)", server_address);
    }
    while(1) {
        int fd = zaccept(sock, sock_type);
        if (fd < 0) {
            zfatal("accept: (%m)");
        }
        void *arg = (void *)((long)fd);
        zcoroutine_go(do_smtpd, arg, 0);
    }
    return 0;
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv, 0);
    server_address = zconfig_get_str(zvar_default_config, "server", 0);
    if (zempty(server_address)) {
        ___usage();
    }
    zcoroutine_base_init();
    zcoroutine_go(do_listen, 0, 0);
    zcoroutine_base_run(0);
    zcoroutine_base_fini();
    return 0;
}
