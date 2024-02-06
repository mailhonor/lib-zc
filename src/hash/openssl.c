/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2024-02-04
 * ================================
 */

#include "zc.h"
#include <openssl/md5.h>

const char *zmd5(const void *data, unsigned len, char *result)
{
    ZSTACK_BUF_FROM(hex_result, result, 32 + 1);
    char buf[16 + 1];
    MD5_CTX c;
    MD5_Init(&c);
    MD5_Update(&c, data, len);
    MD5_Final((unsigned char *)buf, &c);
    zhex_encode(buf, 16, hex_result);
    result[32] = 0;
    zstr_tolower(result);
    return result;
}
