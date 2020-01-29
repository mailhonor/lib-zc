/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-10-12
 * ================================
 */

#include "zc.h"

void test_password(void)
{
    FILE *fp;
    char linebuf[102404];
    zlist_t *list;
    zargv_t *av;

    list = zlist_create();
    fp = fopen("/etc/passwd", "r");
    while (fgets(linebuf, 102400, fp)) {
        av = zargv_create(0);
        zargv_split_append(av, linebuf, ":");
        zlist_push(list, av);
    }
    zinfo("now test ZLIST_WALK_BEGIN");
    ZLIST_NODE_WALK_BEGIN(list, n) {
        zinfo("%s", ((zargv_t *) (n->value))->argv[0]);
    }
    ZLIST_NODE_WALK_END;

    int f=0;
    while(zlist_len(list)) {
        if (f) {
            f = 0;
            zlist_pop(list, 0);
        } else {
            f = 1;
            zlist_shift(list, 0);
        }
    }
    zlist_free(list);
}


typedef struct mystruct mystruct;
struct mystruct {
    int a;
    int b;
};

static void test_mystruct()
{
    zlist_t *list = zlist_create();
    mystruct *ms;

    ms = (mystruct *)malloc(sizeof(mystruct)); ms->a = 1;
    zlist_push(list, ms);

    ms = (mystruct *)malloc(sizeof(mystruct)); ms->a = 2;
    zlist_push(list, ms);

    ms = (mystruct *)malloc(sizeof(mystruct)); ms->a = 8;
    zlist_unshift(list, ms);


    ZLIST_WALK_BEGIN(list, mystruct *, ptr) {
        printf("walk a: %d\n", ptr->a);
    } ZLIST_WALK_END;

    while (zlist_len(list)) {
        if (zlist_pop(list, (void **)&ms)) {
            printf("pop: %d\n", ms->a);
        }
        free(ms);
    }

    zlist_pop(list, (void **)&ms);
    zlist_pop(list, 0);

    zlist_free(list);
}

int main(int argc, char **argv)
{
    test_password();

    test_mystruct();

    return 0;
}
