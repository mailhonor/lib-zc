/*
 * ================================
 * eli960@qq.com
 * http://linxumail.cn/
 * 2017-11-15
 * ================================
 */

#include "zcc/zcc_json.h"

/* 通过API, 生成如下JSON */
/* {
 * "description": "this is json demo, describe json'apis.",
 * "author": "eli960",
 * "thanks": ["you", "he", 123, true 2.01, null],
 * "APIS": {
 *      "constructor": [
 *          "json()",
 *          "json(std::string &str)",
 *          "json(bool val)"
 *          ],
 *      "array_add_element": ["给数组添加一个子节点", "add a new zcc::json element", "nothing"],
 *      "object_add_element": "给对象添加一个子节点"
 *   },
 *   "score": 0.98,
 *   "version": 12,
 *   "published": false
 * }
 */

int main()
{
    std::string result;
    zcc::json json;

    json.object_update("description", "this is json demo, describe json'apis.");
    json.object_update("author", new zcc::json("eli960"));
    json["author"] = "eli960";

    json.object_update("thanks", new zcc::json(), true)
        ->array_add("you")
        ->array_add("he")
        ->array_add(123)
        ->array_add(true)
        ->array_add(2.01)
        ->array_add(new zcc::json());

    json.object_update("APIS", new zcc::json(), true)
        ->object_update("constructor", new zcc::json(), true)
        ->array_add("json()")
        ->array_add("json(std::string &str)")
        ->array_add("json(bool val)")
        ->get_parent()
        ->object_update("array_add", new zcc::json(), true)
        ->array_add(new zcc::json("给数组添加一个子节点"))
        ->array_add("add a new zcc::json element")
        ->array_add("nothing")
        ->get_parent()
        ->object_update("object_update", "给对象添加一个子节点")
        ->get_parent()
        ->object_update("score", 0.98)
        ->object_update("version", new zcc::json(12))
        ->object_update("published", false)
        ->object_update("published2", false, true)
        ->set_string_value("sssss");

    result.clear();
    json.serialize(result, zcc::json_serialize_pretty);
    printf("Json: %s\n", result.c_str());

    zcc::json *cp = json.deep_copy();
    cp->debug_show();
    delete cp;

    json.object_delete("APIS", 0);
    result.clear();
    json.serialize(result, zcc::json_serialize_pretty);
    printf("Json: %s\n", result.c_str());

    return 0;
}

