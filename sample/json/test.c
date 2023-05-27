/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-11-15
 * ================================
 */

#include "zc.h"

/* 通过API, 生成如下JSON */
/* {
 * "description": "this is json demo, describe json'apis.",
 * "author": "eli960",
 * "thanks": ["you", "he", 123, true, 2.01, null],
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
    zjson_t *json = zjson_create(), *tmpj;

    tmpj = zjson_create_string("this is json demo, describe json'apis.", -1);
    zjson_object_update(json, "description", tmpj, 0);
    
    tmpj = zjson_object_update(json, "author", zjson_create_string("eli960", -1), 0);

    tmpj = zjson_object_update(json, "thanks", zjson_create(), 0);
    zjson_array_push(tmpj, zjson_create_string("you", -1));
    zjson_array_push(tmpj, zjson_create_string("he", -1));
    zjson_array_push(tmpj, zjson_create_long(123));
    zjson_array_push(tmpj, zjson_create_bool(1));
    zjson_array_push(tmpj, zjson_create_double(2.01));
    zjson_array_push(tmpj, zjson_create_null());

    tmpj = zjson_object_update(json, "APIS", zjson_create(), 0);
    tmpj = zjson_object_update(tmpj, "constructor", zjson_create(), 0);
    zjson_array_push(tmpj, zjson_create_string("json()", -1));
    zjson_array_push(tmpj, zjson_create_string("json(std::string &str)", -1));
    zjson_array_push(tmpj, zjson_create_string("json(bool val)", -1));

    tmpj = zjson_object_get(json, "APIS");
    if (!tmpj) {
    }
    tmpj = zjson_object_update(tmpj, "array_add_element", zjson_create(), 0);
    zjson_array_push(tmpj, zjson_create_string("给数组添加一个子节点", -1));
    zjson_array_push(tmpj, zjson_create_string("add a new zcc::json element", -1));
    zjson_array_push(tmpj, zjson_create_string("nothing", -1));
#if 0
    zjson_array_insert(tmpj, 100, zjson_create_string("xxx", -1));
    zjson_array_update(tmpj, 20, zjson_create_string("xxx", -1), 0);
#endif

    tmpj = zjson_object_get(json, "APIS");
    if (!tmpj) {
    }
    tmpj = zjson_object_update(tmpj, "object_add_element", zjson_create_string("给对象添加一个子节点", -1), 0);

    zjson_object_update(json, "score", zjson_create_double(0.98), 0);
    zjson_object_update(json, "version", zjson_create_long(12), 0);
    zjson_object_update(json, "published", zjson_create_string(".........", -1), 0);
    zjson_object_update(json, "published", zjson_create_bool(0), 0);

    zbuf_t *result = zbuf_create(-1);

    zjson_serialize(json, result, 0);
    puts(zbuf_data(result));



    zjson_object_delete(json, "version", 0);

    tmpj = 0;
    zjson_object_delete(json, "APIS", &tmpj);

    zbuf_reset(result);
    zjson_serialize(json, result, 0);
    puts(zbuf_data(result));


    zjson_free(json);
    if (tmpj) {
        zjson_free(tmpj);
    }

    zbuf_free(result);

    return 0;
}
