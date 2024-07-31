/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2022-10-20
 * ================================
 */

#include <cstdarg>
#include "zcc/zcc_redis.h"
#include "zcc/zcc_json.h"
#include "zcc/zcc_stream.h"
#include "zcc/zcc_buffer.h"
#include "zcc/zcc_link.h"

#pragma pack(push, 4)
zcc_namespace_begin;

static const int redis_ok = 1;
static const int redis_none = 0;
static const int redis_error = -1;
static const int redis_fatal = -2;

#define zcc_redis_info(...) zcc_info(__VA_ARGS__)
#define zcc_redis_error(...) zcc_error(__VA_ARGS__)
#define zcc_redis_debug(...) \
    if (redis_debug_mode_)   \
    zcc_info(__VA_ARGS__)
#define zcc_redis_debug_read_line(s) \
    if (redis_debug_mode_)           \
    zcc_info("redis 读: %s", s.c_str())
#define zcc_redis_debug_write_line(s) \
    if (redis_debug_mode_)            \
    zcc_info("redis 写: %s", s.c_str())

zcc_namespace_end;
#pragma pack(pop)
