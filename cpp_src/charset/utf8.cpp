/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zcc/zcc_charset.h"

zcc_namespace_begin;
zcc_general_namespace_begin(charset);

/**
 * @brief 检查 UTF-8 字符串尾部是否完整，并返回完整部分的长度
 * 
 * 该函数会从字符串尾部开始检查，确保最后一个 UTF-8 字符是完整的。
 * 如果输入的长度为负数，则会自动计算字符串的长度。
 * 
 * @param ps 指向 UTF-8 字符串的指针
 * @param len 字符串的长度，如果为负数，则会使用 std::strlen 计算长度
 * @return int 完整部分的长度
 */
int utf8_tail_complete(const char *ps, int len)
{
    // 如果传入的长度为负数，则使用 std::strlen 计算字符串的实际长度
    if (len < 0)
    {
        len = std::strlen(ps);
    }
    // 保存原始的长度
    int bak_len = len;

    // 从字符串尾部开始查找 ASCII 字符或者满足 (ch & 0XC0) == 0XC0 的字符
    while (1)
    {
        // 如果长度小于 1，说明已经检查完整个字符串，退出循环
        if (len < 1)
        {
            break;
        }
        // 获取当前位置的字符
        unsigned char ch = ((const unsigned char *)ps)[len - 1];
        // 如果是 ASCII 字符，说明该位置之前的字符都是完整的，返回当前长度
        if (ch < 128)
        {
            return len;
        }
        // 如果满足 (ch & 0XC0) == 0XC0，说明找到了可能的 UTF-8 字符起始字节，退出循环
        if ((ch & 0XC0) == 0XC0)
        {
            break;
        }
        // 不满足条件，继续向前检查
        len--;
    }
    // 如果长度小于 1，说明没有找到完整的 UTF-8 字符，返回当前长度
    if (len < 1)
    {
        return len;
    }

    // 获取当前位置的字符
    unsigned char ch = ((const unsigned char *)ps)[len - 1];
    // 调用 utf8_len 函数获取该字符的字节长度
    int ch_len = utf8_len(ch);
    // 如果从当前位置到字符串末尾的长度足够组成完整的字符，则返回完整字符的长度
    if (bak_len - len >= ch_len - 1)
    {
        return len + ch_len - 1;
    }
    // 否则返回当前长度减 1
    return len - 1;
}

/**
 * @brief 检查 UTF-8 字符串尾部是否完整，并在完整部分末尾添加字符串结束符
 * 
 * 该函数会调用 utf8_tail_complete 函数获取完整部分的长度，
 * 然后在该位置添加字符串结束符 '\0'。
 * 
 * @param ps 指向 UTF-8 字符串的指针
 * @param len 字符串的长度
 * @return char* 指向处理后的字符串的指针
 */
char *utf8_tail_complete_and_terminate(char *ps, int len)
{
    // 调用 utf8_tail_complete 函数获取完整部分的长度
    len = utf8_tail_complete(ps, len);
    // 在完整部分末尾添加字符串结束符
    ps[len] = 0;
    return ps;
}

/**
 * @brief 检查 std::string 类型的 UTF-8 字符串尾部是否完整，并调整字符串长度
 * 
 * 该函数会调用 utf8_tail_complete 函数获取完整部分的长度，
 * 然后调整字符串的长度。
 * 
 * @param s 待处理的 std::string 类型的 UTF-8 字符串
 * @return std::string& 处理后的字符串的引用
 */
std::string &utf8_tail_complete_and_terminate(std::string &s)
{
    // 调用 utf8_tail_complete 函数获取完整部分的长度
    int len = utf8_tail_complete(s.c_str(), s.size());
    // 调整字符串的长度
    s.resize(len);
    return s;
}

/**
 * @brief 获取 UTF-8 字符串的简单摘要
 * 
 * 该函数会根据指定的宽度，从输入的 UTF-8 字符串中提取摘要。
 * 会处理空白字符，将连续的空白字符替换为单个空格。
 * 
 * @param s 指向 UTF-8 字符串的指针
 * @param len 字符串的长度，如果为负数，则会使用 strlen 计算长度
 * @param need_width 摘要所需的宽度
 * @return std::string 提取的摘要字符串
 */
std::string utf8_get_simple_digest(const char *s, int len, int need_width)
{
    // 用于存储提取的摘要字符串
    std::string r;

    // 将输入的字符串指针转换为 unsigned char 类型的指针
    const unsigned char *ps = (const unsigned char *)s;
    // 记录当前摘要的宽度
    int width = 0;
    // 标记上一个字符是否为空白字符
    bool last_blank = true;
    // 如果传入的长度为负数，则使用 strlen 计算字符串的实际长度
    if (len < 0)
    {
        len = strlen(s);
    }
    // 遍历字符串
    for (int i = 0; i < len; i++)
    {
        // 如果摘要宽度超过所需宽度，退出循环
        if (width > need_width - 1)
        {
            break;
        }
        // 获取当前字符
        int ch = ps[i];
        // 调用 zcc::charset::utf8_len 函数获取该字符的字节长度
        int ch_len = zcc::charset::utf8_len(ch);
        // 如果是单字节字符
        if (ch_len == 1)
        {
            // 判断是否为空白字符
            if ((ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n') || (ch < 32))
            {
                // 如果上一个字符也是空白字符，跳过当前字符
                if (last_blank)
                {
                    continue;
                }
                // 否则添加一个空格到摘要中
                r.push_back(' ');
                // 宽度加 1
                width += 1;
                // 标记上一个字符为空白字符
                last_blank = true;
                continue;
            }
            else
            {
                // 非空白字符，添加到摘要中
                r.push_back(ch);
                // 宽度加 1
                width += 1;
                // 标记上一个字符不是空白字符
                last_blank = false;
                continue;
            }
            continue;
        }
        // 如果剩余字符长度不足以组成完整的多字节字符，退出循环
        if (len - i < ch_len)
        {
            break;
        }
        // 如果是双字节字符
        if (ch_len == 2)
        {
            // 判断是否为特殊的空白字符 (0xC2 0xA0)
            if ((ps[i] == 0XC2) && (ps[i + 1] == 0XA0))
            {
                // 如果上一个字符不是空白字符，添加一个空格到摘要中
                if (last_blank)
                {
                }
                else
                {
                    r.push_back(' ');
                    width += 1;
                    last_blank = true;
                }
            }
            else
            {
                // 非特殊空白字符，添加到摘要中
                r.append(s + i, ch_len);
                // 宽度加 2
                width += 2;
                // 标记上一个字符不是空白字符
                last_blank = false;
            }
        }
        else
        {
            // 多字节字符（非双字节），添加到摘要中
            r.append(s + i, ch_len);
            // 宽度加 2
            width += 2;
            // 标记上一个字符不是空白字符
            last_blank = false;
        }
        // 跳过当前多字节字符的剩余字节
        i += ch_len - 1;
    }
    return r;
}

zcc_general_namespace_end(charset);
zcc_namespace_end;
