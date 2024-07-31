/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2020-10-08
 * ================================
 */

#include "zc.h"
#ifdef ZCC_USE_UCONV__
#include <ctype.h>
#include <unicode/utypes.h>
#include <unicode/ucnv.h>
#endif

#ifdef ZCC_USE_UCONV__

#define mydebug(fmt, args...) \
    if (zvar_charset_debug)   \
    {                         \
        zinfo(fmt, ##args);   \
    }

int zcharset_uconv(const char *from_charset, const char *src, int src_len, const char *to_charset, zbuf_t *dest, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count)
{
    int ret = -1;

    zbuf_reset(dest);
    int omit_invalid_bytes_count_tmp = 0;
    if (omit_invalid_bytes_count)
    {
        *omit_invalid_bytes_count = 0;
    }

    from_charset = zcharset_correct_charset(from_charset);
    to_charset = zcharset_correct_charset(to_charset);
    UConverterFromUCallback from_callback = UCNV_FROM_U_CALLBACK_SKIP;
    UConverterToUCallback to_callback = UCNV_TO_U_CALLBACK_SKIP;
    UConverter *from_conv = 0;
    UConverter *to_conv = 0;
    UErrorCode err = U_ZERO_ERROR;

    char *from_p, *from_end;
    char *to_p, *to_end, *to_last;

    zbuf_t *uni_bf = zbuf_create(src_len * 4 + 16);

    int dest_size = src_len * 4 + 16;
    char *dest_buf = (char *)zmalloc(dest_size + 16);

    from_p = (char *)src;
    from_end = (char *)src + src_len;

    err = U_ZERO_ERROR;
    from_conv = ucnv_open(from_charset, &err);
    if (U_FAILURE(err))
    {
        mydebug("cantOpenFromCodeset");
        goto over;
    }
    err = U_ZERO_ERROR;
    ucnv_setToUCallBack(from_conv, to_callback, dest_buf, 0, 0, &err);
    if (U_FAILURE(err))
    {
        mydebug("cantSetCallback");
        goto over;
    }

    while (from_p < from_end)
    {
        to_p = to_last = dest_buf;
        to_end = dest_buf + dest_size;
        err = U_ZERO_ERROR;
        ucnv_toUnicode(from_conv, (UChar **)&to_p, (UChar *)to_end, (const char **)&from_p, from_end, 0, 1, &err);
        if (to_p - to_last > 0)
        {
            zbuf_memcat(uni_bf, to_last, to_p - to_last);
        }

        if (!U_FAILURE(err))
        {
            break;
        }
        char errorBytes[32];
        int8_t errorLength = (int8_t)sizeof(errorBytes);
        UErrorCode localError = U_ZERO_ERROR;
        ucnv_getInvalidChars(from_conv, errorBytes, &errorLength, &localError);
        if (U_FAILURE(localError) || errorLength == 0)
        {
            errorLength = 1;
        }
        omit_invalid_bytes_count_tmp += errorLength;
        if (omit_invalid_bytes_limit == 0)
        {
            break;
        }
        if (omit_invalid_bytes_limit > 0)
        {
            if (omit_invalid_bytes_count_tmp > omit_invalid_bytes_limit)
            {
                break;
            }
        }
    }
    if (src_converted_len)
    {
        *src_converted_len = from_p - src;
    }

    from_p = zbuf_data(uni_bf);
    from_end = zbuf_data(uni_bf) + zbuf_len(uni_bf);
    if (dest_size < zbuf_len(uni_bf) * 4)
    {
        dest_size = zbuf_len(uni_bf) * 4 + 16;
        zfree(dest_buf);
        dest_buf = (char *)zmalloc(dest_size + 16);
    }

    err = U_ZERO_ERROR;
    to_conv = ucnv_open(to_charset, &err);
    if (U_FAILURE(err))
    {
        mydebug("cantOpenToCodeset");
        goto over;
    }
    err = U_ZERO_ERROR;
    ucnv_setFromUCallBack(to_conv, from_callback, from_p, 0, 0, &err);
    if (U_FAILURE(err))
    {
        mydebug("cantSetCallback");
        goto over;
    }

    while (from_p < from_end)
    {
        to_p = to_last = dest_buf;
        to_end = dest_buf + dest_size;
        err = U_ZERO_ERROR;
        ucnv_fromUnicode(to_conv, &to_p, to_end, (const UChar **)&from_p, (const UChar *)from_end, 0, 1, &err);
        if (to_p - to_last > 0)
        {
            zbuf_memcat(dest, to_last, to_p - to_last);
        }
    }
    ret = zbuf_len(dest);

over:
    zbuf_free(uni_bf);
    zfree(dest_buf);
    if (from_conv)
    {
        ucnv_close(from_conv);
    }
    if (to_conv)
    {
        ucnv_close(to_conv);
    }
    if (omit_invalid_bytes_count)
    {
        *omit_invalid_bytes_count = omit_invalid_bytes_count_tmp;
    }
    return ret;
}

void zcharset_convert_use_uconv()
{
    zvar_charset_uconv_mode = 1;
    zcharset_convert = zcharset_uconv;
}

#else /* ZCC_USE_UCONV__ */
#include "zc.h"
static void ___fatal_uconv()
{
    zfatal("unsupported; cmake ../ -DENABLE_ICU_UCONV=yes");
}
int zcharset_uconv(const char *from_charset, const char *src, int src_len, const char *to_charset, zbuf_t *dest, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count)
{
    ___fatal_uconv();
    return -1;
}
void zcharset_convert_use_uconv()
{
    ___fatal_uconv();
}
#endif /* ZCC_USE_UCONV__ */
