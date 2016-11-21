/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-01-19
 * ================================
 */

#include "libzc.h"

int zncr_decode(int ins, char *wchar)
{
    if (ins < 128) {
        *wchar = ins;
        return 1;
    }
    if (ins < 2048) {
        *wchar++ = (ins >> 6) + 192;
        *wchar++ = (ins & 63) + 128;
        return 2;
    }
    if (ins < 65536) {
        *wchar++ = (ins >> 12) + 224;
        *wchar++ = ((ins >> 6) & 63) + 128;
        *wchar++ = (ins & 63) + 128;
        return 3;
    }
    if (ins < 2097152) {
        *wchar++ = (ins >> 18) + 240;
        *wchar++ = ((ins >> 12) & 63) + 128;
        *wchar++ = ((ins >> 6) & 63) + 128;
        *wchar++ = (ins & 63) + 128;
        return 4;
    }

    return 0;
}
