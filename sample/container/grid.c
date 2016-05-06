/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-29
 * ================================
 */

#include "libzc.h"
#include "../rbtree/sysuser.h"

void walk_fn(zgrid_node_t * n, void *ctx)
{
    zinfo("name: %s", zgrid_key(n));
}

int main(int argc, char **argv)
{
    zgrid_t *myos;
    zgrid_node_t *rn;
    sysuser_t *user = 0;
    int i;

    myos = zgrid_create();

    zinfo("load /etc/passwd");
    sysuser_load();

    for (i = 0; i < sysuser_count; i++) {
        zgrid_add(myos, sysuser_list[i].login_name, sysuser_list + i, 0);
    }

    zgrid_node_t *sn;
    if ((sn = zgrid_lookup_near_prev(myos, "paemon", (char **)&user))) {
        zinfo("Found user %s, whose shell is %s", zgrid_key(sn), user->shell);
    } else {
        zinfo("Dit not find the user daemon");
    }

    zinfo("test MACRO of walk");
    ZGRID_WALK_BEGIN(myos, rn) {
        zinfo("name: %s", zdict_key(rn));
    }
    ZGRID_WALK_END;

    zinfo("test walk ...............");
    zgrid_walk(myos, walk_fn, 0);

    zgrid_free(myos, 0, 0);

    sysuser_unload();

    return (0);
}
