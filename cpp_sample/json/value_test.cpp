/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-01-11
 * ================================
 */

#include "zcc_json.h"

static void test_string_one(zcc::json js)
{
    zprintf("str: %s\n", js.get_string_value("DEF").c_str());
}

static void test_string()
{
    zprintf("\n### STRING \n");
    test_string_one(zcc::json("abc"));
    test_string_one(zcc::json(123L));
    test_string_one(zcc::json(false));
    test_string_one(zcc::json(true));
    test_string_one(zcc::json(0.001));
    test_string_one(zcc::json(zcc::json_type_object));
    zprintf("\n");
}

static void test_bool_one(zcc::json js)
{
    zprintf("bool: %d\n", js.get_bool_value(false)?1:0);
}

static void test_bool()
{
    zprintf("\n### bool \n");
    test_bool_one(zcc::json("abc"));
    test_bool_one(zcc::json("True"));
    test_bool_one(zcc::json(123L));
    test_bool_one(zcc::json(0L));
    test_bool_one(zcc::json(false));
    test_bool_one(zcc::json(true));
    test_bool_one(zcc::json(0.001));
    test_bool_one(zcc::json(zcc::json_type_object));
    zprintf("\n");
}

static void test_long_one(zcc::json js)
{
    zprintf("long: %zd\n", js.get_long_value(111));
}

static void test_long()
{
    zprintf("\n### long \n");
    test_long_one(zcc::json("abc"));
    test_long_one(zcc::json("True"));
    test_long_one(zcc::json(123L));
    test_long_one(zcc::json(0L));
    test_long_one(zcc::json(false));
    test_long_one(zcc::json(true));
    test_long_one(zcc::json(0.001));
    test_long_one(zcc::json(zcc::json_type_object));
    zprintf("\n");
}

static void test_double_one(zcc::json js)
{
    zprintf("double: %lf\n", js.get_double_value(111));
}

static void test_double()
{
    zprintf("\n### double \n");
    test_double_one(zcc::json("abc"));
    test_double_one(zcc::json("True"));
    test_double_one(zcc::json(123L));
    test_double_one(zcc::json(0L));
    test_double_one(zcc::json(false));
    test_double_one(zcc::json(true));
    test_double_one(zcc::json(0.001));
    test_double_one(zcc::json(zcc::json_type_object));
    zprintf("\n");
}

static void test_object()
{
    zprintf("\n### object \n");
    zcc::json js;
    zprintf("object/str: %s\n", js.object_get_string_value("one", "111").c_str());
    js["one"] = 123L;
    zprintf("object/str: %s\n", js.object_get_string_value("one", "111").c_str());
    zprintf("object/bool: %d\n", js.object_get_bool_value("one", true));
}

int main(int argc, char **argv)
{
    test_string();
    test_bool();
    test_long();
    test_double();
    test_object();
    return 0;
}

