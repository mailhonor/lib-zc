/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zc.h"

zcc_namespace_begin;

#ifdef _WIN32

int Utf8ToWideChar(const std::string &in, std::wstring &result)
{
    size_t tmplen = in.size() + 16;
    wchar_t *tmp = (wchar_t *)zmalloc(sizeof(wchar_t) * tmplen);
    int r = zUtf8ToWideChar(in.c_str(), in.size(), tmp, tmplen);
    if (r > 0)
    {
        result.append(tmp, r);
    }
    zfree(tmp);
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
    size_t tmplen = in.size() + 16;
    wchar_t *tmp = (wchar_t *)zmalloc(sizeof(wchar_t) * tmplen);
    int r = zMultiByteToWideChar(in.c_str(), in.size(), tmp, tmplen);
    if (r > 0)
    {
        result.append(tmp, r);
    }
    zfree(tmp);
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
    size_t tmplen = in.size()*4 + 16;
    char *tmp = (char *)zmalloc(tmplen);
    int r = zWideCharToUTF8(in.c_str(), in.size(), tmp, tmplen);
    if (r > 0)
    {
        result.append(tmp, r);
    }
    zfree(tmp);
    return r;
}

std::string WideCharToUTF8(const std::wstring &in)
{
    std::string r;
    WideCharToUTF8(in, r);
    return r;
}

int MultiByteToUTF8(const std::string &in, std::string &result)
{
    size_t tmplen = in.size()*4 + 16;
    char *tmp = (char *)zmalloc(tmplen);
    int r = zMultiByteToUTF8(in.c_str(), in.size(), tmp, tmplen);
    if (r > 0)
    {
        result.append(tmp, r);
    }
    zfree(tmp);
    return r;
}

std::string MultiByteToUTF8(const std::string &in)
{
    std::string r;
    MultiByteToUTF8(in, r);
    return r;
}


int WideCharToMultiByte(const std::wstring &in, std::string &result)
{
    size_t tmplen = in.size()*4 + 16;
    char *tmp = (char *)zmalloc(tmplen);
    int r = zWideCharToMultiByte(in.c_str(), in.size(), tmp, tmplen);
    if (r > 0)
    {
        result.append(tmp, r);
    }
    zfree(tmp);
    return r;
}

std::string WideCharToMultiByte(const std::wstring &in)
{
    std::string r;
    WideCharToMultiByte(in, r);
    return r;
}


int UTF8ToMultiByte(const std::string &in, std::string &result)
{
    size_t tmplen = in.size()*4 + 16;
    char *tmp = (char *)zmalloc(tmplen);
    int r = zUTF8ToMultiByte(in.c_str(), in.size(), tmp, tmplen);
    if (r > 0)
    {
        result.append(tmp, r);
    }
    zfree(tmp);
    return r;
}

std::string UTF8ToMultiByte(const std::string &in)
{
    std::string r;
    UTF8ToMultiByte(in, r);
    return r;
}

#endif // _WIN32

zcc_namespace_end;