/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-06
 * ================================
 */

#include "zc.h"

int main(int argc, char **argv)
{
    int i;
    int loop = 1000 * 1000;
    long start, end;
    zmpiece_t *emp;
    char *ptr;

    emp = zmpiece_create(10);
    start = ztimeout_set(0);
    for (i = 0; i < loop; i++) {
        ptr = (char *)zmpiece_alloc_one(emp);
        ptr[1] = i;
        if (i % 100 != 1) {
            zmpiece_free_one(emp, ptr);
        }
    }

    end = ztimeout_set(0);
    printf("%ld\n", end - start);
    zmpiece_free(emp);

    return 0;
}
