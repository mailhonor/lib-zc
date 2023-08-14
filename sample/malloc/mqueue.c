/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-03-17
 * ================================
 */

#include "zc.h"

static void test_1()
{
    zmqueue_t *mq = zmqueue_create(8, 128);
    int i, loop = 10000;
    for (i = 0; i < loop; i++) {
        zmqueue_require_and_push(mq);
    }
    for (i = 0; i < loop; i++) {
        zmqueue_release_and_shift(mq);
    }
    zmqueue_free(mq);
}

static void test_2()
{
    zmqueue_t *mq = zmqueue_create(8, 32);
    int i, j;
    for (j = 0; j < 1000; j++) {
        for (i = 0; i < 10; i++) {
            zmqueue_require_and_push(mq);
        }
        for (i = 0; i < 5; i++) {
            zmqueue_release_and_shift(mq);
        }
    }
    for (j = 0; j < 1000; j++) {
        for (i = 0; i < 5; i++) {
            zmqueue_release_and_shift(mq);
        }
    }
    zmqueue_free(mq);
}

static void test_3()
{
    zmqueue_t *mq = zmqueue_create(8, 128);
    int i, loop = 10000;
    for (i = 0; i < loop; i++) {
        zmqueue_require_and_push(mq);
    }
    for (i = 0; i < loop; i++) {
        zmqueue_require_and_push(mq);
    }
    for (i = 0; i < loop; i++) {
        zmqueue_release_and_shift(mq);
    }
    zmqueue_free(mq);
}

int main(int argc, char **argv)
{
    test_1();
    test_2();
    test_3();
    zprintf("OVER\n");
    return 0;
}

