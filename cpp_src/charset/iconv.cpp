/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zcc/zcc_charset.h"
#include "zcc/zcc_errno.h"
#include <iconv.h>

zcc_namespace_begin;
zcc_general_namespace_begin(charset);

/**
 * @brief 使用 iconv 库将字符串从一种字符集转换为另一种字符集。
 * 
 * 该函数接收源字符集、输入字符串、字符串长度、目标字符集和一个指向整数的指针，
 * 用于存储转换过程中无效字节的数量。它会对输入的字符集名称进行标准化处理，
 * 并根据平台和字符集类型决定是否忽略转换错误。最后，使用 iconv 库进行字符集转换。
 * 
 * @param from_charset 源字符集名称。
 * @param str 输入的字符串。
 * @param str_len 输入字符串的长度，如果为 -1，则自动计算长度。
 * @param to_charset 目标字符集名称。
 * @param invalid_bytes 指向整数的指针，用于存储转换过程中无效字节的数量。
 * @return 转换后的字符串。如果转换失败或输入参数无效，则返回空字符串。
 */
std::string iconv_convert(const char *from_charset, const char *str, int str_len, const char *to_charset, int *invalid_bytes)
{
    // 记录转换过程中无效字节的数量
    int err_bytes = 0;
    // 如果 invalid_bytes 指针不为空，则将其指向的值初始化为 0
    if (invalid_bytes)
    {
        *invalid_bytes = 0;
    }
    // 存储转换后的结果字符串
    std::string r;
    // 如果源字符集或目标字符集为空，则直接返回空字符串
    if (zcc::empty(from_charset) || zcc::empty(to_charset))
    {
        return r;
    }
    // 修正源字符集名称为标准格式
    std::string f_charset = correct_name(from_charset);
    // 修正目标字符集名称为标准格式
    std::string t_charset = correct_name(to_charset);
    // 将源字符集名称转换为大写
    toupper(f_charset);
    // 将目标字符集名称转换为大写
    toupper(t_charset);
    // 如果 invalid_bytes 指针为空，则根据条件决定是否忽略转换错误
    if (!invalid_bytes)
    {
        // 标记是否需要在目标字符集名称后追加 "//IGNORE"
        bool append = false;
#ifdef _WIN64
        // 在 Windows 64 位系统下，默认追加 "//IGNORE"
        append = true;
#endif // _WIN64
        // 获取源字符集名称的 C 风格字符串
        const char *f = f_charset.c_str();
        // 如果不是 Windows 64 位系统，则根据字符集名称判断是否追加 "//IGNORE"
        if (!append)
        {
            // 如果源字符集以 "ISO-2022-" 开头，则追加 "//IGNORE"
            if (!std::strncmp(f, "ISO-2022-", 9))
            {
                append = true;
            }
            // 如果源字符集为 "UTF-7"，则追加 "//IGNORE"
            else if (!std::strcmp(f, "UTF-7"))
            {
                append = true;
            }
        }
        // 如果需要追加，则在目标字符集名称后追加 "//IGNORE"
        if (append)
        {
            t_charset.append("//IGNORE");
        }
    }
    // 更新源字符集名称的指针
    from_charset = f_charset.c_str();
    // 更新目标字符集名称的指针
    to_charset = t_charset.c_str();

    // 打开一个 iconv 转换描述符
    iconv_t ic = iconv_open(to_charset, from_charset);
    // 如果打开失败，则返回空字符串
    if (ic == (iconv_t)-1)
    {
        return "";
    }
    // 如果输入字符串长度为 -1，则自动计算长度
    if (str_len < 0)
    {
        str_len = strlen(str);
    }
    // 如果输入字符串长度为 0，则返回空字符串
    if (str_len == 0)
    {
        return r;
    }

    // 将输入字符串转换为 char* 类型
    char *in_str = (char *)(void *)str;
    // 将输入字符串长度转换为 size_t 类型
    size_t in_len = (size_t)str_len;
    // 计算输出缓冲区的大小
    size_t out_len = in_len * 4 + 16;
    // 分配输出缓冲区内存
    char *out_str = (char *)malloc((int64_t)(out_len + 1));

    // 循环进行字符集转换，直到输入字符串处理完毕
    while (in_len > 0)
    {
        // 重置 iconv 转换状态
        iconv(ic, NULL, NULL, NULL, NULL);
        // 临时保存输出缓冲区的大小和指针
        size_t out_len_tmp = out_len;
        char *out_str_tmp = out_str;
        out_len_tmp = out_len;
        out_str_tmp = out_str;

        // 执行字符集转换
        size_t ret = iconv(ic, &in_str, &in_len, &out_str_tmp, &out_len_tmp);
        // 计算转换后输出字符串的长度
        size_t len = out_str_tmp - out_str;
        // 如果输出字符串长度大于 0，则将其追加到结果字符串中
        if (len > 0)
        {
            r.append(out_str, len);
        }

        // 如果转换成功，则处理剩余的输出
        if (ret != (size_t)-1)
        {
            // 保存当前输出缓冲区的指针
            char *out_str_tmp_last = out_str_tmp;
            // 处理剩余的输出
            iconv(ic, NULL, NULL, &out_str_tmp, &out_len_tmp);
            // 计算剩余输出的长度
            size_t len = out_str_tmp - out_str_tmp_last;
            // 如果剩余输出长度大于 0，则将其追加到结果字符串中dynamic
            if (len > 0)
            {
                r.append(out_str_tmp_last, len);
            }
            // 转换完成，跳出循环
            break;
        }
        // 如果输入字符串处理完毕，则跳出循环
        if (in_len < 1)
        {
            break;
        }
        // 跳过一个无效字节
        in_str++;
        in_len--;
        // 无效字节数量加 1
        err_bytes++;
    }

    // 释放输出缓冲区内存
    free(out_str);
    // 关闭 iconv 转换描述符
    iconv_close(ic);
    // 如果 invalid_bytes 指针不为空，则将无效字节数量赋值给它
    if (invalid_bytes)
    {
        *invalid_bytes = err_bytes;
    }
    // 返回转换后的结果字符串
    return r;
}

zcc_general_namespace_end(charset);
zcc_namespace_end;
