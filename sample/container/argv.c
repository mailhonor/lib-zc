/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-09-29
 * ================================
 */

#include "libzc.h"

static void test_general(void)
{
    zargv_t *ar;

    ar = zargv_create(1);
    zargv_add(ar, "aaa");
    zargv_addn(ar, "bbb", 8);
    zargv_addn(ar, "ccccccc", 3);
    zargv_split_append(ar, "this is good", "iso");
    zargv_show(ar);
    zargv_free(ar);
}

static void test_split(void)
{
    zargv_t *ar;
    char *original = "this is a test sentence.";

    zinfo("==========");
    ar = zargv_create(1);
    zargv_split_append(ar, original, " ");
    zargv_show(ar);
    zargv_free(ar);

    zinfo("==========");
    ar = zargv_create(1);
    zargv_split_append(ar, original, "et");
    zargv_show(ar);
    zargv_free(ar);
}

static void test_mpool(void)
{
    zmpool_t *mpool;
    zargv_t *ar;
    int i;

    mpool = zmpool_create_default_pool(0);
    for (i = 0; i < 1000; i++) {
        ar = zargv_create_mpool(1, mpool);
        zargv_add(ar, "aaa");
        zargv_addn(ar, "bbb", 8);
        zargv_addn(ar, "ccccccc", 3);
        zargv_split_append(ar, "this is good", "iso");
        //zargv_show(ar);
        if (i < 800) {
            zargv_free(ar);
        }
    }

    zmpool_free_pool(mpool);
}

int main(int argc, char **argv)
{
    test_general();
    test_split();
    test_mpool();

    return 0;
}
