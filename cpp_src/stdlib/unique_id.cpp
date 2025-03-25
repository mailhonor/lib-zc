/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2024-02-18
 * ================================
 */

#include "zcc/zcc_stdlib.h"
#include <thread>
#include <ctime>
#include <mutex>
#include <chrono>

zcc_namespace_begin;

/**
 * @brief 构建一个唯一标识符字符串
 * 
 * 该函数通过结合当前时间的纳秒数、线程 ID 和一个自增计数器来生成唯一标识符。
 * 生成的标识符是一个字符串，包含时间戳、线程 ID 和计数器的信息。
 * 
 * @return std::string 生成的唯一标识符字符串
 */
std::string build_unique_id()
{
    // 静态变量，初始化为当前时间的秒级时间戳，用于生成唯一标识符的计数器
    static int64_t _plus = second();
    // 线程局部静态变量，存储当前线程 ID 对 100000 取模的结果
    static thread_local int64_t _tid = get_thread_id() % 100000;
    // 获取当前计数器的值，并将计数器自增
    uint64_t plus = _plus++;
    // 获取当前的高精度时间点
    auto now = std::chrono::high_resolution_clock::now();
    // 计算从纪元开始到当前时间的纳秒数
    int64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();

    // 用于存储格式化字符串的缓冲区
    char buf[32], buf2[32];
    // 将纳秒数格式化为 14 位十六进制字符串，前面补零，并在前面添加两个空格
    std::sprintf(buf, "  %014zx", ns);
    // 将格式化后的字符串复制到结果字符串中
    std::string r = buf;
    // 交换字符串的前两个字符和最后两个字符
    r[0] = r[r.size() - 1];
    r[1] = r[r.size() - 2];
    // 移除字符串的最后两个字符
    r.resize(r.size() - 2);
    // 将线程 ID 格式化为 5 位十六进制字符串，前面补零
    std::sprintf(buf, "%05zx", _tid);
    // 将计数器的后三位格式化为 3 位十六进制字符串，前面补零
    std::sprintf(buf2, "%03zx", plus % 1000);
    // 将格式化后的计数器字符串的最后一个字符添加到结果字符串中
    r.push_back(buf2[2]);
    // 将格式化后的线程 ID 字符串添加到结果字符串中
    r.append(buf);
    // 将格式化后的计数器字符串的第二个字符添加到结果字符串中
    r.push_back(buf2[1]);
    // 将格式化后的计数器字符串的第一个字符添加到结果字符串中
    r.push_back(buf2[0]);
    return r;
}

zcc_namespace_end;
