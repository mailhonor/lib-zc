/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-23
 * ================================
 */

#include "libzc.h"

void zdata_filter_write(void *filter, int type, void *data, int len)
{
    if (type > 0)
    {
        memcpy(filter, data, len);
    }
    else if (type == ZDATA_FILTER_TYPE_ZBUF)
    {
        zbuf_memcat((zbuf_t *) (filter), data, len);
    }
    else if (type == ZDATA_FILTER_TYPE_FILE)
    {
        fwrite(data, 1, len, (FILE *) (filter));
    }
    else if (type == ZDATA_FILTER_TYPE_ZSTREAM)
    {
        zfwrite_n((zstream_t *) (filter), data, len);
    }
}
