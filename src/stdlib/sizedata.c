/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-13
 * ================================
 */

#include "libzc.h"

int zsdata_parse(zsdata_t * sd, void *data, int size)
{
    int i = 0;
    unsigned char *buf = (unsigned char *)data;
    int ch, len = 0, shift = 0;
    while (1) {
        ch = ((i++ == size) ? -1 : *buf++);
        if (ch == -1) {
            return -1;
        }
        len |= ((ch & 0177) << shift);
        if (ch & 0200) {
            break;
        }
        shift += 7;
    }
    if (i + len > size) {
        return -1;
    }
    sd->size = len;
    sd->data = (char *)buf;
    return i + len;
}

void zsdata_escape(zsdata_t * sd, zbuf_t * bf)
{
    zbuf_sizedata_escape(bf, sd->data, sd->size);
}
