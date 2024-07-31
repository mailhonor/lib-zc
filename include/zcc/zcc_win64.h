/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-02-18
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_WIN___
#define ZCC_LIB_INCLUDE_WIN___
#ifdef _WIN64

#include "./zcc_stdlib.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

/* win32 ########################################################### */
int Utf8ToWideChar(const char *in, int in_len, wchar_t *result_ptr, int result_size);
int Utf8ToWideChar(const std::string &in, std::wstring &result);
std::wstring Utf8ToWideChar(const std::string &in);
int MultiByteToWideChar(const char *in, int in_len, wchar_t *result_ptr, int result_size);
int MultiByteToWideChar(const std::string &in, std::wstring &result);
std::wstring MultiByteToWideChar(const std::string &in);
int WideCharToUTF8(const wchar_t *in, int in_size, char *result_ptr, int result_size);
int WideCharToUTF8(const std::wstring &in, std::string &result);
std::string WideCharToUTF8(const std::wstring &in);
int WideCharToUTF8(const wchar_t *in, int in_len, std::string &result);
std::string WideCharToUTF8(const wchar_t *in, int in_len = -1);
int MultiByteToUTF8(const char *in, int in_len, char *result_ptr, int result_size);
int MultiByteToUTF8(const std::string &in, std::string &result);
int MultiByteToUTF8(const char *in, int in_len, std::string &result);
std::string MultiByteToUTF8(const std::string &in);
std::string MultiByteToUTF8(const char *in, int in_len);
int WideCharToMultiByte(const wchar_t *in, int in_len, char *result_ptr, int result_size);
int WideCharToMultiByte(const std::wstring &in, std::string &result);
std::string WideCharToMultiByte(const std::wstring &in);
int Utf8ToMultiByte(const char *in, int in_len, char *result_ptr, int result_size);
int Utf8ToMultiByte(const std::string &in, std::string &result);
std::string Utf8ToMultiByte(const std::string &in);

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // _WIN64
#endif // ZCC_LIB_INCLUDE_WIN___
