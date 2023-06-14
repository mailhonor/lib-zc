/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-11-25
 * ================================
 */

#include "zc.h"

static void usage()
{
    printf("%s [ -server 127.0.0.1:6379 ] -channel news.a1,news.a2,... \n", zvar_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);

    zredis_client_t *rc;
    char *server = zconfig_get_str(zvar_default_config, "server", "127.0.0.1:6379");
    rc = zredis_client_connect(server, 0, 10);
    if (!rc) {
        printf("ERROR can not open %s(%m)\n", server);
        usage();
    }
    zredis_client_set_auto_reconnect(rc, 1);
    char *channel = zconfig_get_str(zvar_default_config, "channel", 0);
    if (zempty(channel)) {
        usage();
    }
    zargv_t *channel_argv = zargv_create(-1);
    zargv_split_append(channel_argv, channel, ",");

    zvector_t *msg_vec = zvector_create(-1);
    if (zredis_client_subscribe(rc, "A", channel_argv)< 1) {
    }

    for (int i = 0; i < 100; i++) {
        zbuf_vector_reset(msg_vec);
        int ret = zredis_client_fetch_channel_message(rc, msg_vec);
        if (ret < 0) {
            printf("ERROR network or protocal\n");
            break;
        } else if (ret == 0) {
            printf("no message\n");
        } else {
            ZVECTOR_WALK_BEGIN(msg_vec, zbuf_t *, bf) {
                printf("%s\n", zbuf_data(bf));
            } ZVECTOR_WALK_END;
            printf("\n");
        }
    }

    printf("\n");

    zargv_free(channel_argv);
    zbuf_vector_free(msg_vec);
    zredis_client_disconnect(rc);

    return 0;
}
