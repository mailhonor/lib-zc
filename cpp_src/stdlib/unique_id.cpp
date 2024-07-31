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

std::string build_unique_id()
{
    static int64_t _plus = second();
    static thread_local int64_t _tid = get_thread_id() % 100000;
    uint64_t plus = _plus++;
    auto now = std::chrono::high_resolution_clock::now();
    int64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();

    char buf[32], buf2[32];
    std::sprintf(buf, "  %014zx", ns);
    std::string r = buf;
    r[0] = r[r.size() - 1];
    r[1] = r[r.size() - 2];
    r.resize(r.size() - 2);
    std::sprintf(buf, "%05zx", _tid);
    std::sprintf(buf2, "%03zx", plus % 1000);
    r.push_back(buf2[2]);
    r.append(buf);
    r.push_back(buf2[1]);
    r.push_back(buf2[0]);
    return r;
}

zcc_namespace_end;
