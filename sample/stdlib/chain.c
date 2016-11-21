/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "libzc.h"

void test_password(void)
{
    FILE *fp;
    char linebuf[102404];
    zchain_t *chain;
    zchain_node_t *n;
    zargv_t *av;

    chain = zchain_create();
    fp = fopen("/etc/passwd", "r");
    while (fgets(linebuf, 102400, fp)) {
        av = zargv_create(0);
        zargv_split_append(av, linebuf, ":");
        zchain_push(chain, av);
    }
    zinfo("now test ZCHAIN_WALK_BEGIN");
    ZCHAIN_WALK_BEGIN(chain, n) {
        zinfo("%s", ((zargv_t *) (n->value))->argv[0]);
    }
    ZCHAIN_WALK_END;
}

int main(int argc, char **argv)
{
    test_password();

    return 0;
}
