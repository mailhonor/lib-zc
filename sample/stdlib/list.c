/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "zc.h"

void test_password(void)
{
    FILE *fp;
    char linebuf[102404];
    zlist_t *list;
    zlist_node_t *n;
    zargv_t *av;

    list = zlist_create();
    fp = fopen("/etc/passwd", "r");
    while (fgets(linebuf, 102400, fp)) {
        av = zargv_create(0);
        zargv_split_append(av, linebuf, ":");
        zlist_push(list, av);
    }
    zinfo("now test ZLIST_WALK_BEGIN");
    ZLIST_WALK_BEGIN(list, n) {
        zinfo("%s", ((zargv_t *) (n->value))->argv[0]);
    }
    ZLIST_WALK_END;
}

int main(int argc, char **argv)
{
    test_password();

    return 0;
}
