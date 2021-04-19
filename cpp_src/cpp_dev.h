/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2020-11-17
 * ================================
 */

#pragma once

#ifndef ___ZC_LIB_CPP_DEV_INCLUDE___
#define ___ZC_LIB_CPP_DEV_INCLUDE___

#ifndef ___ZC_ZCC_MODE___

#define zbuf_data_cpp(bf) zbuf_data(bf)
#define zbuf_len_cpp(bf) zbuf_len(bf)
#define zbuf_put_cpp(bf, ch) zbuf_put(bf, ch)
#define zbuf_reset_cpp(bf) zbuf_reset(bf)
#define zbuf_terminate_cpp(bf) zbuf_terminate(bf)
#define zbuf_truncate_cpp(bf, new_len) zbuf_truncate(bf, new_len)
#define zbuf_strncpy_cpp(bf, src, len) zbuf_strncpy(bf, src, len)
#define zbuf_strcpy_cpp(bf, src) zbuf_strcpy(bf, src)
#define zbuf_strncat_cpp(bf, src, len) zbuf_strncat(bf, src, len)
#define zbuf_strcat_cpp(bf, src) zbuf_strcat(bf, src)
#define zbuf_memcpy_cpp(bf, src, len) zbuf_memcpy(bf, src, len)
#define zbuf_memcat_cpp(bf, src, len) zbuf_memcat(bf, src, len)
#define zbuf_printf_1024_cpp zbuf_printf_1024
#define zbuf_trim_right_rn_cpp(bf) zbuf_trim_right_rn

#define zcharset_convert_cpp zcharset_convert

#else

#define zbuf_data_cpp(bf) ((char *)(void *)bf.c_str())
#define zbuf_len_cpp(bf) ((int)(bf.size()))
#define zbuf_put_cpp(bf, ch) bf.push_back(ch)
#define zbuf_reset_cpp(bf) bf.clear()
#define zbuf_terminate_cpp(bf) 
#define zbuf_truncate_cpp(bf, new_len) XXX
#define zbuf_strncpy_cpp(bf, src, len) (bf.clear(), bf.append(src, len))
#define zbuf_strcpy_cpp(bf, src) (bf.clear(), bf.append(src))
#define zbuf_strncat_cpp(bf, src, len) bf.append(src, len)
#define zbuf_strcat_cpp(bf, src) bf.append(src)
#define zbuf_memcpy_cpp(bf, src, len) (bf.clear(), bf.append(src, len))
#define zbuf_memcat_cpp(bf, src, len) bf.append(src, len)
#define zbuf_printf_1024_cpp sprintf_1024
#define zbuf_trim_right_rn_cpp(bf) XXX

#define zcharset_convert_cpp charset_convert

#endif

#endif /* ___ZC_LIB_CPP_DEV_INCLUDE___ */

