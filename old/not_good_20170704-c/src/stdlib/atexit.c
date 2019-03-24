/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "zc.h"

static zvector_t *___funcs_vec = 0;


void ___before_exit(void)
{
    void (*f)();

    ZVECTOR_WALK_BEGIN(___funcs_vec, f) {
        if(f) {
            f();
        }
    } ZVECTOR_WALK_END;

    zvector_free(___funcs_vec);
}

int zatexit(void (*function)(void))
{
    if (!___funcs_vec) {
        ___funcs_vec = zvector_create(32);
        atexit(___before_exit);
    }
    zvector_add(___funcs_vec, function);

    return 0;
}
