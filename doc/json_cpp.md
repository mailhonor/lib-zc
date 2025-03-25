
## C++ JSON 库, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 支持 json

[LIB-ZC](./README.md) 是 Linux 平台通用 C 扩展库, 内嵌 LIB-ZJSON

LIB-ZJSON C++ 版本 JSON 库, 可独立使用, 代码仓库 https://gitee.com/linuxmail/lib-zjson#readme_md

C 版本 json 库 ../blob/master/doc/json.md

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
zcc::json *js = new zcc::json();
zcc::json *tmpj;
```

### 第二步

```
js->object_update("description", "this is json demo, describe json'apis.");
// (*js)["description"] = "this is json demo, describe json'apis.";

```

得到:

```
{
    "description": "this is json demo, describe json'apis."
}
```

### 第三步

```
js->object_update("author", new zcc::json("http://linuxmail.cn/"));
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
tmpj = js.object_update("thanks", new zcc::json(), true);
tmpj->array_push("you")->array_push("he");
tmpj->array_push(123)->array_push(true);
tmpj->array_push(2.01);
tmpj->array_push(new zcc::json());
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
tmpj = js->object_update("APIS", new json(), true);
tmpj = tmpj->object_update("constructor", new zcc::json(), 0);
tmpj->array_push("json()"));
tmpj->array_push("json(std::string &amp;str)");
tmpj->array_push("json(bool val)");

tmpj = tmpj->object_update("array_add", new zcc::json(), true);
tmpj->array_push("给数组添加一个子节点");
tmpj->array_push(new zcc::json("add a new zcc::json element"));
tmpj->array_push("nothing");

tmpj = tmpj->object_update("object_add", new zcc::json("给对象添加一个子节点"));
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
js->object_update("score", 0.98);
js->object_update("version", 12L);
js->object_update("published", new zcc::json(".........XXX"));
js->object_update("published", new zcc::json("........."), &tmpj);
delete (tmpj);
js->object_update("published", false);
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
delete js;
```

## 方法: 构造

### json()

* 创建 json, 得到 null

### json(const std::string &val);

* 创建 json, 字符串类型, 其值复制自 val

### json(const char *val, int len = -1);

* 创建 json, 字符串类型, 其值复制自长度为 len 的 val

### json(long val);

* 创建 json, long 类型, 其值为 long

### json(bool val);

* 创建 json, bool 类型, 其值为 val

### json(const unsigned char type);

* 创建 json, 类型为 type

## json 类型

### json_type_null<BR />json_type_bool<BR />json_type_string<BR />json_type_long<BR />json_type_double<BR />json_type_object<BR />json_type_array<BR />json_type_unknown

* json 类型

### int get_type();

* inline, 返回 json 类型

### bool is_null();<BR />bool is_bool();<BR />bool is_long();<BR />bool is_double();<BR />bool is_string();<BR />bool is_object();<BR />bool is_array();

* 返回是否"某类型"

## 方法: json 的值

### bool &get_bool_value(bool def = false);

* 获取 bool 值
* 如果不是 bool 类型, 则首先转换为值为 bool 类型, 其值:
  * 原来是 null, 则为 def
  * 原来是 string, 则:
    * "t*" => true
    * "T*" => true
    * "y*" => true
    * "Y*" => true
    * "f*" => false
    * "F*" => false
    * "n*" => false
    * "N*" => false
    * "1" => true
    * "0" => false
    * 其他 => def
  * 原来是 long/double, 则: 0 为 false, 否则 true
  * 原来是 array, 则为 def
  * 原来是 object, 则为 def

### ssize_t &get_long_value(ssize_t def = 0);

* 获取 long 值
* 如果不是 long 类型, 则首先转换为 long 类型, 其值:
  * 原来是 null, 则为 def
  * 原来是 string, 则为 atol(val)
  * 原来是 bool, 则为 0 或  1
  * 原来是 double, 则为 long(val)
  * 原来是 array, 则为 def
  * 原来是 object, 则为 def

### double &get_double_value(double def = 0.0);

* 获取 double 值
* 如果不是 double 类型, 则首先转换为值为 double 类型, 其值:
  * 原来是 null, 则为 def
  * 原来是 string, 则为 atof(val)
  * 原来是 bool, 则为 0.0 或  1.0
  * 原来是 long, 则为 打印结果
  * 原来是 array, 则为 def
  * 原来是 object, 则为 def

### std::string &get_string_value(const char *def = "");

* 获取 string 值
* 如果不是 string 类型, 则首先转换为 string 类型, 其值:
  * 原来是 null, 则为 def
  * 原来是 bool, 则为 "0" 或  "1"
  * 原来是 long, 则为 打印结果
  * 原来是 double, 则为 打印结果
  * 原来是 array, 则为 def
  * 原来是 object, 则为 def

### const std::vector&lt;json *&gt; &get_array_value();

* 获取数组值
* 如果不是数组类型, 则首先转换为值为 [] 的数组类型

### const std::map&lt;std::string, json *&gt; &get_object_value();

* 获取对象值
* 如果不是对象类型, 则首先转换为值为 {} 的对象类型

## 方法: 对象{}操作

下面的操作, 如果不是对象类型, 则首先转换为值为 {} 的对象类型

### const std::map&lt;std::string, json *&gt; &get_object_value();

* 获取对象值;

### json *object_get(const char *key);

* 获取键为 key 的子 json

### int object_size();

* 获取子 json 个数 

### json *object_update(const char *key, json *element, json **old_element, bool return_child = flase);<BR />json_t *object_update(onst char *key, json *element, bool return_child = false);

* 增加或更新键为 key 对应的 json, 新值为 element
* 旧值如果存在则赋值给 *old_element, 如果 old_element 为 0 则销毁

### json *object_delete(const char *key, json **old_element);

* 移除键 key 及对应的 json
* 如果存在则其值赋值给 *old_element, 如果 old_element为了 0 则销毁

### std::string object_get_string_value(const std::string &key, const char *def = ""); <BR />ssize_t object_get_long_value(const std::string &key, ssize_t def = 0); <BR />bool object_get_bool_value(const std::string &key, bool def = false);
* 扩展
* 如果不是 object 类型, 则先转为 object 类型
* 获取子对象的值, 见 上面的 get_string_value 等

## 方法: 数组操作

下面操作, 如果不是数组,先转为值为 [] 的数组

### json *array_get(int idx);

* 获取下标为 idx 的 子 json

### int array_size(json *j);

* 获取数组长度

### json *array_push(json *element, return_child = false);<BR />json *array_add(json *element, return_child = false);

* 在数组后追加 element(json)

### bool array_pop(json **element);

* 最后一个子 json 存在, 则弹出; 返回 true; 这个子 json 赋值给 *element, 如果 element==0 则销毁
* 如果最后一个子 json 不存在, 返回 false

### json *array_unshift(json *element, return_child = false);

* 在数组前追加 element(json)

### bool array_shift(json **element);

* 第一个子 json 存在, 则弹出; 返回 true; 这个子 json 赋值给 *element, 如果 element==0 则销毁
* 第一个子 json 不存在, 返回 false

### json *array_update(int idx, json *element, json **old_element, bool return_child = false);<BR />json *array_update(int idx, json *j, bool return_child = false);

* 更新下标为 idx 的子 json 为 element
* 旧 json 赋值给 *old_element, old_element==0 则销毁

#### 举例说明:

* 假设 json: [1, {}, "ss" "aaa"]
* 如果执行: array_update(6, element)<BR />则, 新json: [1, {}, "ss", "aaa", null, null, element]
* 如果执行: array_update(2, element, &amp;old_element)<BR />则, 新json: [1, {}, element, "aaa"], 且: old_element赋值为 "ss"

### json *array_insert(int idx, json *element);

* 把 element 插入 idx 前

### bool array_delete(int idx, json **old_element);

* 删除下标为 idx的节点, 存在则返回true, 赋值给 *old_element, 如果 old_element为 0, 则销毁
* 其后节点顺序迁移

## 函数: path

### json *get_by_path(const char *path);

#### 例子

* 假设 json: {group:{linux:[{}, {}, {me: {age:18, sex:"male"}}}}
* 执行: json.get_by_path("group/linux/2/me");
* 返回: {age:18, sex:"male"}

### json *get_by_path_vec(json *j, const char *path0, .../* , 0 */);

#### 例子

* 假设 json: {group:{linux:[{}, {}, {me: {age:18, sex:"male"}}}}
* 执行: json.get_by_path_vec(json, "group", "linux", "2", "me", 0);
* 返回: {age:18, sex:"male"}

## 操作符重载

举例子:

```
// 对象
zcc::json js;
js["abc"] = "xxx1";
js["def"] = false;
// 数组
zcc::json js;
js[12] = new json();
js[1] = "xxx2";
// 自身
zcc::json js = "abc";
js = 123;
```

## 方法: 反序列化/序列化/深度复制

### bool unserialize(const char *jstr, int jlen);

* 反序列化长度为 jlen 的 jstr, 把反序列化结果覆盖到 j
* 返回 成功/失败

例子:
```
zcc::json js;
const char *s =  "{\"errcode\": \"-801\", \"errmsg\": \"Domain Not Exist\"}\r\n";
js.unserialize(s, -1);
```

### json *serialize(std::string &result, int flag = 0);

* 对 j 做序列化, 结果存储(追加)到 result
* flag 支持 json_serialize_strict | json_serialize_pretty
* json_serialize_strict: 严谨, 尽量满足规范
* json_serialize_pretty: 格式化文本


### json *deep_copy();

* 深度复制


## 例子

* https://gitee.com/linuxmail/lib-zjson/blob/master/

