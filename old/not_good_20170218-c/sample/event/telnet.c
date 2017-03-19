/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-04
 * ================================
 */

#include "libzc.h"
#include <openssl/ssl.h>

static int use_ssl = 0;
static char *dest = 0;

static void usage(void)
{

    printf("%s [ -ssl] host:port\n", zvar_progname);
    printf("%s [ -ssl] local_unix_domain_path\n", zvar_progname);
    exit(0);
}

void after_close(void *ctxt)
{
    printf("\nclose\n");

    /* 由于本测试只有一个链接，所以断开后，考虑终止程序 */
    exit(0);
}

int do_parameter(int argc, char **argv)
{
    char *n = argv[0];
    if (!strcmp(n, "-ssl")) {
        use_ssl = 1;
        return 1;
    }
    dest = n;
    return 1;
}

int main(int argc, char **argv)
{
    ziopipe_base_t *iopb;
    zsslctx_t *sslctx;
    zssl_t *ssl;
    SSL *openssl = 0;
    int dest_fd;

    zvar_progname = argv[0];

    zparameter_run(argc - 1, argv + 1, do_parameter);

    if (!dest) {
        usage();
    }

    iopb = ziopipe_base_create();

    dest_fd = zconnect(dest, 10 * 1000);
    if (dest_fd < 0) {
        printf("     fail\n");
        exit(0);
    }
    printf("     success\n");

    znonblocking(dest_fd, 1);
    if (use_ssl) {
        zssl_INIT(0);
        sslctx = zsslctx_client_create(0);
        ssl = zssl_create(sslctx, dest_fd);
        if (zssl_connect(ssl, 1000) < 0) {
            printf("error: ssl connect\n");
        }
        openssl = zssl_detach_ssl(ssl);
        zssl_free(ssl);
    }
    znonblocking(0, 1);

    ziopipe_enter(iopb, 0, 0, dest_fd, openssl, after_close, 0);

    ziopipe_base_run(iopb);

    /* release */

    return 0;
}
