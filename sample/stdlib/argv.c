/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-09-29
 * ================================
 */

#include "zc.h"

static void test_general(void)
{
    zargv_t *ar;

    ar = zargv_create(1);
    zargv_add(ar, "aaa");
    zargv_addn(ar, "bbb", 8);
    zargv_addn(ar, "ccccccc", 3);
    zargv_split_append(ar, "this is good", "iso");
    zargv_debug_show(ar);
    zargv_free(ar);
}

static void test_split(void)
{
    zargv_t *ar;
    const char *original = "this is a test sentence.";

    zinfo("==========");
    ar = zargv_create(1);
    zargv_split_append(ar, original, " ");
    zargv_debug_show(ar);
    zargv_free(ar);

    zinfo("==========");
    ar = zargv_create(1);
    zargv_split_append(ar, original, "et");
    zargv_debug_show(ar);
    zargv_free(ar);

    zinfo("==========");
    ar = zargv_create(1);
    zargv_split_append(ar, original, "XXX");
    zargv_debug_show(ar);
    zargv_free(ar);

    zinfo("==========");
    ar = zargv_create(1);
    zargv_split_append(ar, "", "et");
    zargv_debug_show(ar);
    zprintf("count:%d\n", zargv_len(ar));
    zargv_free(ar);
}

int main(int argc, char **argv)
{
    test_general();
    test_split();

    return 0;
}
