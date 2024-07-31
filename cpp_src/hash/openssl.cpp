/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2024-02-04
 * ================================
 */

#include "zcc/zcc_stdlib.h"
#include <openssl/md5.h>

zcc_namespace_begin;

std::string md5(const void *data, unsigned int len)
{
    std::string result;
    char buf[16 + 1];
    MD5_CTX c;
    MD5_Init(&c);
    MD5_Update(&c, data, len);
    MD5_Final((unsigned char *)buf, &c);
    hex_encode(buf, 16, result);
    return result;
}

zcc_namespace_end;
