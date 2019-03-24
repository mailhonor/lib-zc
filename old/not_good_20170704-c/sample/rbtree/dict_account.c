/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-21
 * ================================
 */

#include "zc.h"
#include "sysuser.h"

int main(int argc, char **argv)
{
    zdict_t *myos;
    zdict_node_t *rn;
    sysuser_t *user = 0;
    int i;

    myos = zdict_create();

    zinfo("load /etc/passwd");
    sysuser_load();

    for (i = 0; i < sysuser_count; i++) {
        zdict_update(myos, sysuser_list[i].login_name, sysuser_list + i, 0);
    }

    if (zdict_find(myos, "daemon", (char **)&user)) {
        zinfo("Found user daemon, whose shell is %s", user->shell);
    } else {
        zinfo("Dit not find the user daemon");
    }

    zinfo("test MACRO of walk");
    ZDICT_WALK_BEGIN(myos, rn) {
        zinfo("name: %s", ZDICT_KEY(rn));
    }
    ZDICT_WALK_END;

    zinfo("test walk ...............");

    zdict_free(myos);

    sysuser_unload();

    return (0);
}
