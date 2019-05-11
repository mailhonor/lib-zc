/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-10-21
 * ================================
 */

#include "zc.h"
#include "sysuser.h"

int main(int argc, char **argv)
{
    zdict_t *myos;
    zbuf_t *shell;
    int i;

    myos = zdict_create();

    zinfo("load /etc/passwd");
    sysuser_load();

    for (i = 0; i < sysuser_count; i++) {
        zdict_update_string(myos, sysuser_list[i].login_name, sysuser_list[i].shell, -1);
    }

    if (zdict_find(myos, "daemon", &shell)) {
        zinfo("Found user daemon, whose shell is %s", zbuf_data(shell));
    } else {
        zinfo("Dit not find the user daemon");
    }

    zinfo("test MACRO of walk");
    ZDICT_NODE_WALK_BEGIN(myos, rn) {
        zinfo("name: %s, home: %s", zdict_node_key(rn), zbuf_data(zdict_node_value(rn)));
    }
    ZDICT_NODE_WALK_END;

    zinfo("test walk ...............");

    zdict_free(myos);

    sysuser_unload();

    return (0);
}
