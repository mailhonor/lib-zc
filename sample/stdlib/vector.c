/*
 * ================================
 * eli960@qq.com
 * www.mailhonor.com
 * 2018-12-21
 * ================================
 */

#include "zc.h"

int main(int argc, char **argv)
{
    zvector_t *vec = zvector_create(-1);
    int i;

    for (i = 0; i < 1000; i++) {
        zvector_push(vec, 0);
    }

    for (i = 0; i < 100; i++) {
        zvector_shift(vec, 0);
    }

    for (i = 0; i < 1000; i++) {
        zvector_unshift(vec, 0);
    }

    for (i = 0; i < 100; i++) {
        zvector_pop(vec, 0);
    }

    zvector_insert(vec, 10000, 0);

    while(zvector_len(vec) > 1) {
        zvector_delete(vec, 1, 0);
    }
    zvector_delete(vec, 0, 0);

    zvector_free(vec);

    return 0;
}
