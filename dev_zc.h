/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-14
 * ================================
 */

#pragma once

#ifndef ___ZC_LIB_DEV_INCLUDE___
#define ___ZC_LIB_DEV_INCLUDE___

// #define WINAPI_FAMILY WINAPI_PARTITION_APP

#define ZC_ROBUST_DO(exp)      \
    int ret = -1;              \
    while (1)                  \
    {                          \
        ret = exp;             \
        if (ret > -1)          \
        {                      \
            return ret;        \
        }                      \
        int ec = zget_errno(); \
        if (ec == EINTR)       \
        {                      \
            continue;          \
        }                      \
        errno = ec;            \
        return ret;            \
    }                          \
    return ret;

#define ZC_ROBUST_DO_WIN32(exp) \
    int ret = -1;               \
    while (1)                   \
    {                           \
        ret = exp;              \
        if (ret > -1)           \
        {                       \
            return ret;         \
        }                       \
        int ec = zget_errno();  \
        if (ec == EINTR)        \
        {                       \
            continue;           \
        }                       \
        errno = ec;             \
        return ret;             \
    }                           \
    return ret;

#endif //___ZC_LIB_DEV_INCLUDE___
