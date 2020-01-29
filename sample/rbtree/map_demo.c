/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2019-11-05
 * ================================
 */

#include "zc.h"

typedef struct mystruct mystruct;
struct mystruct {
    int a;
    int b;
};

int main(int argc, char **argv)
{
    zmap_t *map = zmap_create();
    mystruct *ms;

    ms = (mystruct *)malloc(sizeof(mystruct)); ms->a = 1;
    zmap_update(map, "key1", ms, 0);

    ms = (mystruct *)malloc(sizeof(mystruct)); ms->a = 2;
    zmap_update(map, "key2", ms, 0);

    ms = (mystruct *)malloc(sizeof(mystruct)); ms->a = 3;
    zmap_update(map, "key3", ms, 0);

    ms = (mystruct *)malloc(sizeof(mystruct)); ms->a = 5;
    /* 因为 key2 已经存在, 但第四个参数为 0, 所以 ms->a == 2的 mystruct 内存泄露了 */
    zmap_update(map, "key2", ms, 0);

    ms = (mystruct *)malloc(sizeof(mystruct)); ms->a = 6;
    mystruct *ms2;
    /* 尽管 key3 已经存在, 但第四个参数为ms2, 所以 ms->a == 3的 mystruct 赋值给了 ms2  */
    zmap_update(map, "key3", ms, (void **)&ms2);
    if (ms2) {
        free(ms2);
    }

    ZMAP_WALK_BEGIN(map, key, mystruct *, ms6) {
        printf("key=%s, a=%d\n", key, ms6->a);
        free(ms6);
    } ZMAP_WALK_END;


    zmap_free(map);

    return 0;
}
