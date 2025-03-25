
## JSON 库, [LIB-ZC](./README.md)

[C++ 版本 json 库](./json_cpp.md)

[LIB-ZC](./README.md) 支持 JSON, 其 STRUCT 类型是 **zjson_t**

## 数据结构

```
struct zjson_t {
    union {
        zbool_t b;
        long l;
        double d;
        zbuf_t *s;
        zvector_t *v; /* <zjson_t *>             */
        zmap_t *m;    /* <char *, zjson_t *>     */
    } val;
    zjson_t *parent;
    unsigned char type;
};
```

## 生成一个 json

下面演示通过API逐步生成一个 json

### 目标 json:

```
{
    "description": "this is json demo, describe json'apis.",
    "author": "http://linuxmail.cn",
    "thanks": ["you", "he", 123, true, 2.01, null],
    "APIS": {
        "constructor": [ "json()", "json(std::string &amp;str)", "json(bool val)" ],
        "array_add_element": ["给数组添加一个子节点", "add a new zcc::json element", "nothing"],
        "object_add_element": "给对象添加一个子节点"
    },
    "score": 0.98,
    "version": 12,
    "published": false
}
```

### 第一步, 创建一个json

```
zjson_t *json = zjson_create();
zjson_t *tmpj;
```

### 第二步

```
tmpj = zjson_create_string("this is json demo, describe json'apis.", -1);
zjson_object_update(json, "description", tmpj, 0);
```

得到:

```
{
    "description": "this is json demo, describe json'apis."
}
```

### 第三步

```
tmpj = zjson_object_update(json, "author", zjson_create_string("http://linuxmail.cn/", -1), 0);
```

得到

```
{
    "description": "this is json demo, describe json'apis.",
    "author": "http://linuxmail.cn/"
}
```

### 第四步

```
tmpj = zjson_object_update(json, "thanks", zjson_create(), 0);
zjson_array_push(tmpj, zjson_create_string("you", -1));
zjson_array_push(tmpj, zjson_create_string("he", -1));
zjson_array_push(tmpj, zjson_create_long(123));
zjson_array_push(tmpj, zjson_create_bool(1));
zjson_array_push(tmpj, zjson_create_double(2.01));
zjson_array_push(tmpj, zjson_create_null());
```

得到

```
{
    "description": "this is json demo, describe json'apis.",
    "author": "http://linuxmail.cn/",
    "thanks": ["you", "he", 123, true, 2.01, null]
}
```

### 第五步

```
tmpj = zjson_object_update(json, "APIS", zjson_create(), 0);
tmpj = zjson_object_update(tmpj, "constructor", zjson_create(), 0);
zjson_array_push(tmpj, zjson_create_string("json()", -1));
zjson_array_push(tmpj, zjson_create_string("json(std::string &amp;str)", -1));
zjson_array_push(tmpj, zjson_create_string("json(bool val)", -1));

tmpj = zjson_object_update(tmpj, "array_add_element", zjson_create(), 0);
zjson_array_push(tmpj, zjson_create_string("给数组添加一个子节点", -1));
zjson_array_push(tmpj, zjson_create_string("add a new zcc::json element", -1));
zjson_array_push(tmpj, zjson_create_string("nothing", -1));

tmpj = zjson_object_update(tmpj, "object_add_element", zjson_create_string("给对象添加一个子节点", -1), 0);
```

得到:

```
{
    "description": "this is json demo, describe json'apis.",
    "author": "http://linuxmail.cn/",
    "thanks": ["you", "he", 123, true, 2.01, null],
    "APIS": { "constructor": [ "json()", "json(std::string &amp;str)", "json(bool val)" ],
        "array_add_element": ["给数组添加一个子节点", "add a new zcc::json element", "nothing"],
        "object_add_element": "给对象添加一个子节点"
    }
}
```

### 第六步

```
zjson_object_update(json, "score", zjson_create_double(0.98), 0);
zjson_object_update(json, "version", zjson_create_long(12), 0);
zjson_object_update(json, "published", zjson_create_string(".........", -1), 0);
zjson_object_update(json, "published", zjson_create_bool(0), 0);
```

得到最终目标

```
{
    "description": "this is json demo, describe json'apis.",
    "author": "http://linuxmail.cn",
    "thanks": ["you", "he", 123, true, 2.01, null],
    "APIS": {
        "constructor": [ "json()", "json(std::string &amp;str)", "json(bool val)" ],
        "array_add_element": ["给数组添加一个子节点", "add a new zcc::json element", "nothing"],
        "object_add_element": "给对象添加一个子节点"
    },
    "score": 0.98,
    "version": 12,
    "published": false
}
```

### 第七步, 释放json

```
zjson_free(json);
```

## 函数: 基本操作

### zjson_t *zjson_create();

* 创建 json, 返回 null

### zjson_t *zjson_create_null();

* 创建 json, 返回 null

### zjson_t *zjson_create_bool(zbool_t b);

* 创建 json, bool 类型
* 如果 (b==0) 则 返回 True 否则 返回 False

### zjson_t *zjson_create_long(long l);

* 创建 json, long 整数类型, 值为 l

### zjson_t *zjson_create_double(double d);

* 创建 json, 浮点数类型, 值为 d

### zjson_t *zjson_create_string(const void *s, int len);

* 创建 json, 字符串类型
* 如果 len&lt;0, (逻辑上)值为: strdup((const char *)s);
* 如果 len&gt;=0, (逻辑上)值为: memdup(s, len);

## 函数: 类型

### zvar_json_type_null<BR />zvar_json_type_bool<BR />zvar_json_type_string<BR />zvar_json_type_long<BR />zvar_json_type_double<BR />zvar_json_type_object<BR />zvar_json_type_array<BR />zvar_json_type_unknown

* json 类型, 宏(整数)

### int zjson_get_type(zjson_t *j);

* inline, 返回 json 类型

### zbool_t zjson_is_null(zjson_t *j);<BR />zbool_t zjson_is_bool(zjson_t *j);<BR />zbool_t zjson_is_long(zjson_t *j);<BR />zbool_t zjson_is_double(zjson_t *j);<BR />zbool_t zjson_is_string(zjson_t *j);<BR />zbool_t zjson_is_object(zjson_t *j);<BR />zbool_t zjson_is_array(zjson_t *j);

* 返回是否"某类型"

## 函数: json 的值

### zbool_t *zjson_get_bool_value(zjson_t *j);

* 获取 bool 值的地址
* 如果不是 bool 类型, 则首先转换为值为 0(false) 的 bool 类型

### long *zjson_get_long_value(zjson_t *j);

* 获取 long 值的地址
* 如果不是 long 类型, 则首先转换为值为 0 的long 类型

### double *zjson_get_double_value(zjson_t *j);

* 获取 double 值的地址
* 如果不是 double 类型, 则首先转换为值为 0 的 double 类型

### zbuf_t **zjson_get_string_value(zjson_t *j);

* 获取(zbuf_t *)值的地址
* 如果不是(zbuf_t *)类型, 则首先转换为值为 "" 的(zbuf_t *)类型

### const zvector_t *zjson_get_array_value(zjson_t *j);

* 获取数组值的地址
* 如果不是数组类型, 则首先转换为值为 [] 的数组类型

### const zmap_t *zjson_get_object_value(zjson_t *j);

* 获取对象值的地址
* 如果不是对象类型, 则首先转换为值为 {} 的对象类型

## 函数: 对象{}操作

下面的操作, 如果不是对象类型, 则首先转换为值为 {} 的对象类型

### const zmap_t *zjson_get_object_value(zjson_t *j); /* &lt;char *, zjson_t *&gt; */

* 获取对象值的地址;

### zjson_t *zjson_object_get(zjson_t *j, const char *key);

* 获取键为 key 的子 json

### int zjson_object_get_len(zjson_t *j);

* 获取子 json 个数 

### zjson_t *zjson_object_update(zjson_t *j, const char *key, zjson_t *element, zjson_t **old_element);<BR />zjson_t *zjson_object_add(zjson_t *j, const char *key, zjson_t *element, zjson_t **old_element);

* 增加或更新键为 key 对应的 json, 新值为 element
* 旧值如果存在则赋值给 *old_element, 如果 old_element 为 0 则销毁

### void zjson_object_delete(zjson_t *j, const char *key, zjson_t **old_element);

* 移除键 key 及对应的 json
* 如果存在则其值赋值给 *old_element, 如果 old_element为了 0 则销毁

## 函数: 数组操作

下面操作, 如果不是数组,先转为值为 [] 的数组

### zjson_t *zjson_array_get(zjson_t *j, int idx);

* 获取下标为 idx 的 子 json

### int zjson_array_get_len(zjson_t *j);

* 获取数组长度

### zjson_t *zjson_array_push(zjson_t *j, zjson_t *element);<BR />zjson_t *zjson_array_add(zjson_t *j, zjson_t *element);

* 在数组后追加 element(json), 返回 element

### zbool_t zjson_array_pop(zjson_t *j, zjson_t **element);

* 最后一个子 json 存在, 则弹出; 返回 1; 这个子 json 赋值给 *element, 如果 element==0 则销毁
* 最后一个子 json 不存在, 返回 0

### zjson_t *zjson_array_unshift(zjson_t *j, zjson_t *element);

* 在数组前追加 element(json), 返回 element

### zbool_t zjson_array_shift(zjson_t *j, zjson_t **element);

* 第一个子 json 存在, 则弹出; 返回 1; 这个子 json 赋值给 *element, 如果 element==0 则销毁
* 第一个子 json 不存在, 返回 0

### zjson_t *zjson_array_update(zjson_t *j, int idx, zjson_t *element, zjson_t **old_element);

* 更新下标为 idx 的子 json 为 element, 返回 element
* 旧 json 赋值给 *old_element, old_element==0 则销毁

#### 举例说明:

* 假设 json: [1, {}, "ss" "aaa"]
* 如果执行: zjson_array_update(json, 6, element, 0)<BR />则, 新json: [1, {}, "ss", "aaa", null, null, 6]
* 如果执行: zjson_array_update_element(json, 2, element, &amp;old_element)<BR />则, 新json: [1, {}, element, "aaa"], 且: old_element赋值为 "ss"
* 如果执行: zjson_array_update_element(json, 2, element, 0)<BR />则, 新json: [1, {}, element, "aaa"], 且: 销毁 "ss" 
### zjson_t *zjson_array_insert(zjson_t *j, int idx, zjson_t *element);

* 把 element 插入 idx 处, 原来的 idx 及其后元素顺序后移

### void zjson_array_delete(zjson_t *j, int idx, zjson_t **old_element);

* 移除 idx 处 json, 并把其值付给 *old_element; idx 后元素属性前移

## 函数: path

### zjson_t *zjson_get_element_by_path(zjson_t *j, const char *path);

#### 例子

* 假设 json: {group:{linux:[{}, {}, {me: {age:18, sex:"male"}}}}
* 执行: zjson_get_element_by_path(json, "group/linux/2/me");
* 返回: {age:18, sex:"male"}

### zjson_t *zjson_get_element_by_path_vec(zjson_t *j, const char *path0, ...);

#### 例子

* 假设 json: {group:{linux:[{}, {}, {me: {age:18, sex:"male"}}}}
* 执行: zjson_get_element_by_path_vec(json, "group", "linux", "2", "me", 0);
* 返回: {age:18, sex:"male"}

## 函数: 反序列化

### zbool_t zjson_unserialize(zjson_t *j, const char *jstr, int jlen);

* 反序列化长度为 jlen 的 jstr, 把反序列化结果覆盖到 j
* 返回 0: 失败
* 返回 1: 成功;

#### 例子:
```
zjson_t *j = zjson_create();
const char *s =  "{\"errcode\": \"-801\", \"errmsg\": \"Domain Not Exist\"}\r\n";
zjson_unserialize(j, s, -1);
```

## 函数: 序列化

### void zjson_serialize(zjson_t *j, zbuf_t *result, int strict);

* 对 j 做序列化, 结果存储(追加)到 result; strict 使用 0

## 例子

* [goto](../sample/json/)

