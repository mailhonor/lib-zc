/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#ifdef _WIN64
#include "zcc/zcc_win64.h"

zcc_namespace_begin;

int Utf8ToWideChar(const char *in, int in_len, wchar_t *result_ptr, int result_size)
{
    wchar_t *result_buf = (wchar_t *)result_ptr;
    result_buf[0] = L'\0';
    int ret_len = ::MultiByteToWideChar(CP_UTF8, 0, in, in_len, NULL, 0);
    if (ret_len < 1)
    {
        return -1;
    }
    ret_len = ::MultiByteToWideChar(CP_UTF8, 0, in, in_len, result_buf, result_size);
    if (ret_len < 1)
    {
        return -1;
    }
    if (in_len < 0)
    {
        result_buf[ret_len - 1] = L'\0';
        return ret_len - 1;
    }
    else
    {
        result_buf[ret_len] = L'\0';
        return ret_len;
    }
}

// UTF-16 (wide character)
// 返回结果是字符数
// result_size 是 结果(result_ptr)buffer 的字符数
// in_len 是 输入(in) 的字节数
// https://learn.microsoft.com/zh-cn/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
int MultiByteToWideChar(const char *in, int in_len, wchar_t *result_ptr, int result_size)
{
    int codepage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
    wchar_t *result_buf = (wchar_t *)result_ptr;
    result_buf[0] = L'\0';
    int ret_len = ::MultiByteToWideChar(codepage, 0, in, in_len, NULL, 0);
    if (ret_len < 1)
    {
        return -1;
    }
    ret_len = ::MultiByteToWideChar(codepage, 0, in, in_len, result_buf, result_size);
    if (ret_len < 1)
    {
        return -1;
    }
    if (in_len < 0)
    {
        result_buf[ret_len - 1] = L'\0';
        return ret_len - 1;
    }
    else
    {
        result_buf[ret_len] = L'\0';
        return ret_len;
    }
}

// 返回结果是字节数
// in_len 是输入(in)的字符数
// result_size 是结果(result_ptr)buffer的字节数
// https://learn.microsoft.com/zh-cn/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte
int WideCharToUTF8(const wchar_t *in, int in_len, char *result_ptr, int result_size)
{
    char *result_buf = (char *)result_ptr;
    result_buf[0] = 0;
    int ret_len = ::WideCharToMultiByte(CP_UTF8, 0, in, in_len, 0, 0, 0, 0);
    if (ret_len < 1)
    {
        return -1;
    }
    ret_len = ::WideCharToMultiByte(CP_UTF8, 0, in, in_len, result_buf, ret_len, 0, 0);
    if (ret_len < 1)
    {
        return -1;
    }
    if (in_len < 0)
    {
        result_buf[ret_len - 1] = 0;
        return ret_len - 1;
    }
    else
    {
        result_buf[ret_len] = 0;
        return ret_len;
    }
}

int WideCharToMultiByte(const wchar_t *in, int in_len, char *result_ptr, int result_size)
{
    int codepage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
    char *result_buf = (char *)result_ptr;
    result_buf[0] = 0;
    int ret_len = ::WideCharToMultiByte(codepage, 0, in, in_len, 0, 0, 0, 0);
    if (ret_len < 1)
    {
        return -1;
    }
    ret_len = ::WideCharToMultiByte(codepage, 0, in, in_len, result_buf, ret_len, 0, 0);
    if (ret_len < 1)
    {
        return -1;
    }
    if (in_len < 0)
    {
        result_buf[ret_len - 1] = 0;
        return ret_len - 1;
    }
    else
    {
        result_buf[ret_len] = 0;
        return ret_len;
    }
}

int Utf8ToMultiByte(const char *in, int in_len, char *result_ptr, int result_size)
{
    wchar_t *unicode_ptr = new wchar_t[result_size + 16];
    int unicode_len = Utf8ToWideChar(in, in_len, unicode_ptr, result_size);
    if (unicode_len < 1)
    {
        delete[] unicode_ptr;
        return -1;
    }
    int ret = WideCharToMultiByte(unicode_ptr, unicode_len, result_ptr, result_size);
    delete[] unicode_ptr;
    return ret;
}

int Utf8ToWideChar(const std::string &in, std::wstring &result)
{
    uint64_t tmplen = in.size() + 16;
    wchar_t *tmp = new wchar_t[tmplen];
    int r = Utf8ToWideChar(in.c_str(), in.size(), tmp, tmplen);
    if (r > 0)
    {
        result.append(tmp, r);
    }
    delete[] tmp;
    return r;
}

std::wstring Utf8ToWideChar(const std::string &in)
{
    std::wstring r;
    Utf8ToWideChar(in, r);
    return r;
}

int MultiByteToWideChar(const std::string &in, std::wstring &result)
{
    uint64_t tmplen = in.size() + 16;
    wchar_t *tmp = new wchar_t[tmplen];
    int r = MultiByteToWideChar(in.c_str(), in.size(), tmp, tmplen);
    if (r > 0)
    {
        result.append(tmp, r);
    }
    delete[] tmp;
    return r;
}

std::wstring MultiByteToWideChar(const std::string &in)
{
    std::wstring r;
    MultiByteToWideChar(in, r);
    return r;
}

int WideCharToUTF8(const std::wstring &in, std::string &result)
{
    uint64_t tmplen = in.size() * 4 + 16;
    char *tmp = new char[tmplen];
    int r = WideCharToUTF8(in.c_str(), in.size(), tmp, tmplen);
    if (r > 0)
    {
        result.append(tmp, r);
    }
    delete[] tmp;
    return r;
}

int WideCharToUTF8(const wchar_t *in, int in_len, std::string &result)
{
    if (in_len < 0)
    {
        in_len = std::wcslen(in);
    }
    uint64_t tmplen = in_len * 4 + 16;
    char *tmp = new char[tmplen];
    int r = WideCharToUTF8(in, in_len, tmp, tmplen);
    if (r > 0)
    {
        result.append(tmp, r);
    }
    delete[] tmp;
    return r;
}

std::string WideCharToUTF8(const std::wstring &in)
{
    std::string r;
    WideCharToUTF8(in, r);
    return r;
}

std::string WideCharToUTF8(const wchar_t *in, int in_len)
{
    std::string r;
    WideCharToUTF8(in, in_len, r);
    return r;
}

int MultiByteToUTF8(const char *in, int in_len, char *result_ptr, int result_size)
{
    wchar_t *unicode_ptr = new wchar_t[result_size + 16];
    int unicode_len = MultiByteToWideChar(in, in_len, unicode_ptr, result_size);
    if (unicode_len < 1)
    {
        delete[] unicode_ptr;
        return -1;
    }
    int ret = WideCharToUTF8(unicode_ptr, unicode_len, result_ptr, result_size);
    delete[] unicode_ptr;
    return ret;
}

int MultiByteToUTF8(const char *in, int in_len, std::string &result)
{
    uint64_t tmplen = in_len * 4 + 16;
    char *tmp = new char[tmplen];
    int r = MultiByteToUTF8(in, in_len, tmp, tmplen);
    if (r > 0)
    {
        result.append(tmp, r);
    }
    delete[] tmp;
    return r;
}

int MultiByteToUTF8(const std::string &in, std::string &result)
{
    int r = MultiByteToUTF8(in.c_str(), (int)in.size(), result);
    return r;
}

std::string MultiByteToUTF8(const char *in, int in_len)
{
    std::string result;
    MultiByteToUTF8(in, in_len, result);
    return result;
}

std::string MultiByteToUTF8(const std::string &in)
{
    std::string r;
    MultiByteToUTF8(in, r);
    return r;
}

int WideCharToMultiByte(const std::wstring &in, std::string &result)
{
    uint64_t tmplen = in.size() * 4 + 16;
    char *tmp = new char[tmplen];
    int r = WideCharToMultiByte(in.c_str(), in.size(), tmp, tmplen);
    if (r > 0)
    {
        result.append(tmp, r);
    }
    delete[] tmp;
    return r;
}

std::string WideCharToMultiByte(const std::wstring &in)
{
    std::string r;
    WideCharToMultiByte(in, r);
    return r;
}

int Utf8ToMultiByte(const std::string &in, std::string &result)
{
    uint64_t tmplen = in.size() * 4 + 16;
    char *tmp = new char[tmplen];
    int r = Utf8ToMultiByte(in.c_str(), in.size(), tmp, tmplen);
    if (r > 0)
    {
        result.append(tmp, r);
    }
    delete[] tmp;
    return r;
}

std::string Utf8ToMultiByte(const std::string &in)
{
    std::string r;
    Utf8ToMultiByte(in, r);
    return r;
}

zcc_namespace_end;

#endif // _WIN64
