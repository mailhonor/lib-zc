/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-08-23
 * ================================
 */

#include "zcc_json.h"

int main(int argc, char **argv)
{
    zcc::json js, *tmpjs;

    do {
        /* 准备 js */
        js.array_add(1)->array_add(false)->array_add("333")->array_push(555);
        /* 得到 [ 1, false, "333", 555 ] */
        js.debug_show();
    } while (0);

    do {
        /* 在最前面插入 "begin" 和 second */
        js.array_unshift("begin")->array_insert(1, "second");
        /* 得到 [ "begin", "second", 1, false, "333", 555 ] */
        js.debug_show();
    } while (0);

    do {
        /* 删除最后两个节点 */
        js.array_pop(0);
        js.array_delete(-1, 0);
        /* 删除第 2 个节点 */
        js.array_delete(1, 0);
        /* 得到 [ "begin", 1, false ] */
        js.debug_show();
    } while (0);

    do {
        /* 在第 6 个 节点 添加 2.0 */
        js.array_update(5, 2.0);
        /* 得到 [ "begin", 1, false, null, null, 2.0 ] */
        js.debug_show();
    } while (0);

    while (js.array_pop(&tmpjs)) {
        /* 弹出第一个节点 */
        if (tmpjs) {
            delete tmpjs;
        }
        js.debug_show();
    }

    return 0;
}

