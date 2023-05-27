/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-12-07
 * ================================
 */

//grow type mem pool does not support realloc

#include "zc.h"

void test_default_mpool()
{
    int i;
    long start, end;
    zmpool_t *mp;
    char *ptr;

    mp = zmpool_create_common_pool(0);
    ptr = 0;
    start = ztimeout_set(0);
    for (i = 0; i < 1000000; i++) {
        ptr = (char *)zmpool_realloc(mp, ptr, (i + 1) % 4096);
        if (i % 5 == 0) {
            zmpool_free(mp, ptr);
            ptr = 0;
        }
    }
    zmpool_free(mp, ptr);

    end = ztimeout_set(0);
    printf("%ld\n", end - start);

    zmpool_free_pool(mp);

}

void test_grow_mpool()
{
    int i, j;
    long start, end;
    zmpool_t *mp;
    char *ptr;

    start = ztimeout_set(0);

    for (j = 0; j < 10; j++) {
        mp = zmpool_create_greedy_pool(1024*1024, 1024*100);
        ptr = 0;
        for (i = 0; i < 100000; i++) {
            ptr = (char *)zmpool_malloc(mp, (i + 1) % 4096);
            if (i % 5 == 0) {
                zmpool_free(mp, ptr);
            }
        }
        zmpool_free_pool(mp);
    }

    end = ztimeout_set(0);
    printf("%ld\n", end - start);
}

int main(int argc, char **argv)
{
    test_default_mpool();
    test_grow_mpool();

    return 0;
}
