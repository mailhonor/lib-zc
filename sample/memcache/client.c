/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-11-25
 * ================================
 */

#include "zc.h"

int main(int argc, char **argv)
{
    zmemcache_client_t *mc = 0;
    zbuf_t *str = 0;
    long l;
    int flag;
    char *server;

    zmain_argument_run(argc, argv);
    str = zbuf_create(1024);
    server = zconfig_get_str(zvar_default_config, "server", "127.0.0.1:11211");
    do {
        if ((mc = zmemcache_client_connect(server, 10)) == 0) {
            printf("ERROR can not connect %s(%m)\n", server);
            break;
        }
#if 0
        zmemcache_client_set_cmd_timeout(mc, 10);
        zmemcache_client_set_auto_reconnect(mc, 1);
#endif

        if (zmemcache_client_set(mc, "iii", 0, 0, "123", 3) < 0) {
            printf("ERROR zmemcache_client_set iii\n");
            break;
        }
        printf("zmemcache_client_set iii 123\n");

        l = zmemcache_client_incr(mc, "iii", 3);
        if (l < 0) {
            printf("ERROR zmemcache_client_incr iii\n");
            break;
        }
        printf("zmemcache_client_incr iii: %ld\n", l);

        if (zmemcache_client_get(mc, "iii", &flag, str) < 0) {
            printf("ERROR zmemcache_client_get iii\n");
            break;
        }
        printf("zmemcache_client_get iii %s\n", zbuf_data(str));

        if (zmemcache_client_version(mc, str) < 0) {
            printf("ERROR zmemcache_client_version\n");
            break;
        }
        printf("zmemcache_client_version %s\n", zbuf_data(str));

    } while(0);
    printf("USAGE: %s [ -server 127.0.0.1:11211 ]\n", argv[0]);
    zbuf_free(str);
    if (mc) {
        zmemcache_client_disconnect(mc);
    }
    return 0;
}
