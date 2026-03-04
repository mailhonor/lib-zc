/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */

// 这是 icu 的核心转码引擎, 本库用的就是这个函数
// 使用者可以复制后修改, 依赖只有 zcc::charset::correct_name

#include "zcc/zcc_charset.h"
#include <unicode/utypes.h>
#include <unicode/ucnv.h>

#ifndef uconv_convert_engine_debug
#define uconv_convert_engine_debug
#endif

extern "C"
{
    static std::string uconv_convert_engine(const char *from_charset, const char *src, int64_t src_len, const char *to_charset, int64_t *invalid_bytes)
    {
        std::string dest;
        int err_bytes = 0;
        if (invalid_bytes)
        {
            *invalid_bytes = 0;
        }

        from_charset = zcc::charset::correct_name(from_charset);
        to_charset = zcc::charset::correct_name(to_charset);
        UConverterFromUCallback from_callback = UCNV_FROM_U_CALLBACK_SKIP;
        UConverterToUCallback to_callback = UCNV_TO_U_CALLBACK_SKIP;
        UConverter *from_conv = 0;
        UConverter *to_conv = 0;
        UErrorCode err = U_ZERO_ERROR;
        std::string uni_bf;

        const char *from_p, *from_end;
        const char *to_p, *to_end, *to_last;

        int64_t dest_size = src_len * 4 + 16;
        char *dest_buf = new char[dest_size + 16];

        from_p = src;
        from_end = src + src_len;

        err = U_ZERO_ERROR;
        from_conv = ucnv_open(from_charset, &err);
        if (U_FAILURE(err))
        {
            uconv_convert_engine_debug("cantOpenFromCodeset");
            goto over;
        }
        err = U_ZERO_ERROR;
        ucnv_setToUCallBack(from_conv, to_callback, dest_buf, 0, 0, &err);
        if (U_FAILURE(err))
        {
            uconv_convert_engine_debug("cantSetCallback");
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
                uni_bf.append(to_last, to_p - to_last);
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
            err_bytes += errorLength;
        }

        from_p = uni_bf.c_str();
        from_end = from_p + uni_bf.size();
        if (dest_size < (int64_t)uni_bf.size() * 4)
        {
            dest_size = (int)uni_bf.size() * 4 + 16;
            delete[] dest_buf;
            dest_buf = new char[dest_size + 16];
        }

        err = U_ZERO_ERROR;
        to_conv = ucnv_open(to_charset, &err);
        if (U_FAILURE(err))
        {
            uconv_convert_engine_debug("cantOpenToCodeset");
            goto over;
        }
        err = U_ZERO_ERROR;
        ucnv_setFromUCallBack(to_conv, from_callback, from_p, 0, 0, &err);
        if (U_FAILURE(err))
        {
            uconv_convert_engine_debug("cantSetCallback");
            goto over;
        }

        while (from_p < from_end)
        {
            to_p = to_last = dest_buf;
            to_end = dest_buf + dest_size;
            err = U_ZERO_ERROR;
            ucnv_fromUnicode(to_conv, (char **)&to_p, to_end, (const UChar **)&from_p, (const UChar *)from_end, 0, 1, &err);
            if (to_p - to_last > 0)
            {
                dest.append(to_last, to_p - to_last);
            }
        }

    over:
        uni_bf.clear();
        delete[] dest_buf;
        if (from_conv)
        {
            ucnv_close(from_conv);
        }
        if (to_conv)
        {
            ucnv_close(to_conv);
        }
        if (invalid_bytes)
        {
            *invalid_bytes = err_bytes;
        }
        return dest;
    }
}