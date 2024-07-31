/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2016-01-15
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_INTPACK___
#define ZCC_LIB_INCLUDE_INTPACK___

#include "./zcc_stdlib.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

inline unsigned int int_unpack(const void *buf)
{
    unsigned char *p = (unsigned char *)buf;
    unsigned int n = p[0];
    n <<= 8;
    n |= p[1];
    n <<= 8;
    n |= p[2];
    n <<= 8;
    n |= p[3];
    return n;
}

inline void int_pack(unsigned int num, void *buf)
{
    unsigned char *p = (unsigned char *)buf;
    p[3] = num & 255;
    num >>= 8;
    p[2] = num & 255;
    num >>= 8;
    p[1] = num & 255;
    num >>= 8;
    p[0] = num & 255;
}

inline unsigned int int_unpack3(const void *buf)
{
    unsigned char *p = (unsigned char *)buf;
    unsigned int n = p[0];
    n <<= 8;
    n |= p[1];
    n <<= 8;
    n |= p[2];
    return n;
}

inline void int_pack3(unsigned int num, void *buf)
{
    unsigned char *p = (unsigned char *)buf;
    p[2] = num & 255;
    num >>= 8;
    p[1] = num & 255;
    num >>= 8;
    p[0] = num & 255;
}

inline unsigned int int_unpack2(const void *buf)
{
    unsigned char *p = (unsigned char *)buf;
    unsigned int n = p[0];
    n <<= 8;
    n |= p[1];
    return n;
}

inline void int_pack2(unsigned int num, void *buf)
{
    unsigned char *p = (unsigned char *)buf;
    p[1] = num & 255;
    num >>= 8;
    p[0] = num & 255;
}

int cint_data_unescape(const void *src_data, int src_size, void **result_data, int *result_len);
int cint_data_unescape_all(const void *src_data, int src_size, size_data *vec, int vec_size);

void cint_data_escape(std::string &bf, const void *data, int len);
void cint_data_escape_int(std::string &bf, int i);
void cint_data_escape_long(std::string &bf, int64_t i);
void cint_data_escape_pp(std::string &bf, const char **pp, int size);
int cint_put(int size, char *buf);

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_INTPACK___
