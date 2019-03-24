/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-09-04
 * ================================
 */

#include "zc.h"

static int _close(zfinder_t * finder)
{
    return 0;
}

static int _get(zfinder_t * finder, const char *query, zbuf_t * result, long timeout)
{
    zbuf_strcpy(result, (char *)(finder->title) + 9);
    return 1;
}

int zfinder_create_static(zfinder_t *finder)
{
    finder->close = _close;
    finder->get = _get;

    return 0;
}
