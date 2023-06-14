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
    zcc::json js;

    do {
        /* 准备 js */
        js.object_update("str", "string2");
        js.object_update("str", "string");
        js.object_update("int", 2LL);
        js.object_update("bool", false);
        /* 得到 {"bool":false,"int":2,"str":"string"} */
        js.debug_show();
    } while (0);

    do {
        /* 更改 "bool" 的值为 true */
        js.object_get("bool")->get_bool_value() = true;
        /* 得到 {"bool":ture,"int":2,"str":"string"} */
        js.debug_show();
    } while (0);

    do {
        /* 更改 "int" 的值为 3 */
        js.object_update("int", 3LL);
        /* 得到 {"bool":ture,"int":3,"str":"string"} */
        js.debug_show();
    } while (0);

    do {
        /* 更改 "str" 的值为 {} */
        js.object_update("str", new zcc::json(zcc::json_type_object));
        /* 得到 {"bool":ture,"int":3,"str":{}} */
        js.debug_show();
    } while (0);

    do {
        /* 增加 "array" => [ 1, false ] */
        js.object_update("array", new zcc::json(), true)->array_push(1LL)->array_push(false);
        /* 得到 {"array":[1,false],"bool":true,"int":3,"str":{}} */
        js.debug_show();
    } while (0);

    return 0;
}

