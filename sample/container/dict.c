/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-29
 * ================================
 */

#include "libzc.h"
#include "../rbtree/sysuser.h"

void walk_fn(zdict_node_t * n, void *ctx)
{
    zinfo("name: %s", zdict_key(n));
}

int main(int argc, char **argv)
{
    zdict_t *myos;
    zdict_node_t *rn;
    char *shell;
    int i;

    myos = zdict_create();

    zinfo("load /etc/passwd");
    sysuser_load();

    for (i = 0; i < sysuser_count; i++)
    {
        zdict_add(myos, sysuser_list[i].login_name, sysuser_list[i].shell);
    }

    zdict_node_t *sn;
    if ((sn = zdict_lookup_near_prev(myos, "paemon", &shell)))
    {
        zinfo("Found user %s, whose shell is %s", zdict_key(sn), shell);
    }
    else
    {
        zinfo("Dit not find the user daemon");
    }

    zinfo("test MACRO of walk");
    ZDICT_WALK_BEGIN(myos, rn)
    {
        zinfo("name: %s", zdict_key(rn));
    }
    ZDICT_WALK_END;

    zinfo("test walk ...............");
    zdict_walk(myos, walk_fn, 0);

    zdict_free(myos);

    sysuser_unload();

    return (0);
}
