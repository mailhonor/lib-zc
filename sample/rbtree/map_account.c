/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-10-21
 * ================================
 */

#include "zc.h"
#include "sysuser.h"

int main(int argc, char **argv)
{
    zmap_t *myos;
    sysuser_t *user = 0;
    int i;

    myos = zmap_create();

    zinfo("load /etc/passwd");
    sysuser_load();

    for (i = 0; i < sysuser_count; i++) {
        zmap_update(myos, sysuser_list[i].login_name, sysuser_list + i, 0);
    }

    if (zmap_find(myos, "daemon", (void **)&user)) {
        zinfo("Found user daemon, whose shell is %s", user->shell);
    } else {
        zinfo("Dit not find the user daemon");
    }

    zinfo("test MACRO of walk");
    ZMAP_WALK_BEGIN(myos, key, sysuser_t *, user) {
        zinfo("name: %s, home: %s", key, user->home);
    } ZMAP_WALK_END;

    zinfo("test walk ...............");

    zmap_free(myos);

    sysuser_unload();

    return (0);
}
