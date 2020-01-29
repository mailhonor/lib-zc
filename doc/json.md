# JSON 库

[LIB-ZC](https://gitee.com/linuxmail/lib-zc)是一个C扩展库. 支持 JSON


##  生成json

已知 json, 通过API逐步生成这个json

```
{
"description": "this is json demo, describe json'apis.",
"author": "eli960",
"thanks": ["you", "he", 123, true, 2.01, null],
"APIS": {
     "constructor": [
         "json()",
         "json(std::string &str)",
         "json(bool val)"
         ],
     "array_add_element": ["给数组添加一个子节点", "add a new zcc::json element", "nothing"],
     "object_add_element": "给对象添加一个子节点"
  },
  "score": 0.98,
  "version": 12,
  "published": false
}
```

**第一步**

 初始一个json

```
zjson_t *json = zjson_create();
zjson_t  *tmpj;
```

**第二步** 

 ```
tmpj = zjson_create_string("this is json demo, describe json'apis.", -1);
zjson_object_update(json, "description", tmpj, 0);
 ```

得到

```
{
 "description": "this is json demo, describe json'apis."
 }
 ```

**第三步** 

```
/* tmpj 等于 zjson_create_string("eli960", -1) */
tmpj = zjson_object_update(json, "author", zjson_create_string("eli960", -1), 0);
```

得到

```
{
"description": "this is json demo, describe json'apis.",
"author": "eli960"
}
```

**第四步**
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
"author": "eli960",
"thanks": ["you", "he", 123, true, 2.01, null],
}
```

**第五步**

```
tmpj = zjson_object_update(json, "APIS", zjson_create(), 0);
tmpj = zjson_object_update(tmpj, "constructor", zjson_create(), 0);
zjson_array_push(tmpj, zjson_create_string("json()", -1));
zjson_array_push(tmpj, zjson_create_string("json(std::string &str)", -1));
zjson_array_push(tmpj, zjson_create_string("json(bool val)", -1));

tmpj = zjson_object_update(tmpj, "array_add_element", zjson_create(), 0);
zjson_array_push(tmpj, zjson_create_string("给数组添加一个子节点", -1));
zjson_array_push(tmpj, zjson_create_string("add a new zcc::json element", -1));
zjson_array_push(tmpj, zjson_create_string("nothing", -1));

tmpj = zjson_object_update(tmpj, "object_add_element", zjson_create_string("给对象添加一个子节点", -1), 0);
```

得到

```
{
"description": "this is json demo, describe json'apis.",
"author": "eli960",
"thanks": ["you", "he", 123, true, 2.01, null],
"APIS": {
     "constructor": [
         "json()",
         "json(std::string &str)",
         "json(bool val)"
         ],
     "array_add_element": ["给数组添加一个子节点", "add a new zcc::json element", "nothing"],
     "object_add_element": "给对象添加一个子节点"
   }
  }
```

**第六步**

```
zjson_object_update(json, "score", zjson_create_double(0.98), 0);
zjson_object_update(json, "version", zjson_create_long(12), 0);
zjson_object_update(json, "published", zjson_create_string(".........", -1), 0);
zjson_object_update(json, "published", zjson_create_bool(0), 0);
```

得到最终目标

**第七步**

释放json

```
zjson_free(json);
```

##  解析字符串

解析下面的字符为json

```
const char *s =  "{\"errcode\": \"-801\", \"errmsg\": \"Domain Not Exist\"}\r\n";
```

代码如下 

```
zjson_t *j = zjson_create();
/* 解析函数 */
zjson_unserialize(j, s, -1);
/* j 就是解析结果 */
```

## json序列化

```
zjson_t *j = zjson_create();
zbuf_t *result = zbuf_create(-1);
/* ... */

/* 序列化函数 */
zjson_serialize(j,  result, 0);
/* 序列化结果保存在result */
```

##  创建json

```
/* 创建json */
zjson_t *zjson_create();

/* 创建undefined/null */
#define zjson_create_null zjson_create

/* 创建bool */
zjson_t *zjson_create_bool(zbool_t b);

/* long */
zjson_t *zjson_create_long(long l);

/* doube */
zjson_t *zjson_create_double(double d);

/* string */
zjson_t *zjson_create_string(const void *s, int len);
```

## json的类型

```
zinline int zjson_get_type(zjson_t *j) ;
zinline zbool_t zjson_is_null(zjson_t *j);
zinline zbool_t zjson_is_bool(zjson_t *j) ;
zinline zbool_t zjson_is_long(zjson_t *j) ;
zinline zbool_t zjson_is_double(zjson_t *j) ;
zinline zbool_t zjson_is_string(zjson_t *j) ;
zinline zbool_t zjson_is_object(zjson_t *j);
zinline zbool_t zjson_is_array(zjson_t *j) ;
```

## json的值

```
/* 获取bool值的指针; 如果不是bool类型,则首先转换为bool类型, 默认为0*/
zbool_t *zjson_get_bool_value(zjson_t *j);

/* 获取long值的指针; 如果不是long类型, 则首先转换为long类型, 默认为 0 */
long *zjson_get_long_value(zjson_t *j);

/* 获取double值的指针; 如果不是double类型, 则首先转换为long类型, 默认为 0 */
double *zjson_get_double_value(zjson_t *j);

/* 获取(zbuf_t *)值的指针; 如果不是zbuf_t *类型, 则首先转换为zbuf_t *类型, 值默认为 "" */
zbuf_t **zjson_get_string_value(zjson_t *j);

/* 获取数组值的指针; 如果不是数组类型, 则首先转换为数组类型, 默认为 [] */
const zvector_t *zjson_get_array_value(zjson_t *j); /* <zjson_t *> */

/* 获取对象值的指针; 如果不是对象类型, 则首先转换为对象类型, 默认为 {} */
const zmap_t *zjson_get_object_value(zjson_t *j); /* <char *, zjson_t *> */
```

## 对象操作

```
/* 获取对象值的指针; 如果不是对象类型, 则首先转换为对象类型, 默认为 {} */
const zmap_t *zjson_get_object_value(zjson_t *j); /* <char *, zjson_t *> */

/* 如果不是对象,先转为对象, 获取下键为key的子json */
zjson_t *zjson_object_get(zjson_t *j, const char *key);

/* 如果不是对象,先转为对象, 获取子json个数 */
int zjson_object_get_len(zjson_t *j);

/* 增加或更新键为key对应的json, 新值为element;
 * 旧值如果存在则赋值给*old_element, 如果old_element为了0则销毁  */
zjson_t *zjson_object_update(zjson_t *j, const char *key, zjson_t *element, zjson_t **old_element);
#define zjson_object_add zjson_object_update

/* 移除键key及对应的json;
 * json如果存在则赋值给*old_element, 如果old_element为了0则销毁  */
void zjson_object_delete(zjson_t *j, const char *key, zjson_t **old_element);
```

## 数组操作

```
/* 如果不是数组,先转为数组, 获取下表为idx的 子json */
zjson_t *zjson_array_get(zjson_t *j, int idx);

/* 如果不是数组,先转为数组, 获取数组长度 */
int zjson_array_get_len(zjson_t *j);

/* 如果不是数组,先转为数组, 在数组后追加element(json). 返回element */
zjson_t *zjson_array_push(zjson_t *j, zjson_t *element);
#define zjson_array_add zjson_array_push

/* 如果不是数组,先转为数组, 存在则返回1, 否则返回 0;
 * element不为0,则pop出来的json赋值给*element, 否则销毁 */
zbool_t zjson_array_pop(zjson_t *j, zjson_t **element);

/* 如果不是数组,先转为数组, 存在则返回1, 否则返回 0;
 * element不为0,则unshift出来的json赋值给*element, 否则销毁 */
zjson_t *zjson_array_unshift(zjson_t *j, zjson_t *element);

/* 如果不是数组,先转为数组, 在数组前追加element(json). 返回element */
zbool_t zjson_array_shift(zjson_t *j, zjson_t **element);

/* 已知 json = [1, {}, "ss" "aaa"]
 * 1, zjson_array_update 给键idx设置成员element. 返回element
 * 2, 如果键idx不存在, 则直接赋值
 *    2.1, 例子: zjson_array_update(json, 6, element, 0)
 *         结果: [1, {}, "ss", "aaa", null, null, 6]
 * 3, 如果键idx存在
 *    3.1, 把旧值赋值给 *old_element, 如果old_element为0,则销毁.
 *         再做element的赋值
 *    3.2, 例子: zjson_array_update_element(json, 2, element, &old_element)
 *         结果: [1, {}, element, "aaa"], 且 *old_element 为 "ss"
 *    3.3, 例子: zjson_array_update_element(json, 2, element, 0);
 *         结果: [1, {}, element, "aaa"], 且 销毁 "ss" */
zjson_t *zjson_array_update(zjson_t *j, int idx, zjson_t *element, zjson_t **old_element);

/* 把element插入idx处, idx及其后元素顺序后移 */
zjson_t *zjson_array_insert(zjson_t *j, int idx, zjson_t *element);

/* 移除idx处json,并把其值付给 *old_element, idx后元素属性前移 */
void zjson_array_delete(zjson_t *j, int idx, zjson_t **old_element);
```

## path

```
/* 已知json {group:{linux:[{}, {}, {me: {age:18, sex:"male"}}}}, 则
 * zjson_get_element_by_path(json, "group/linux/2/me") 返回的 应该是 {age:18, sex:"male"} */
zjson_t *zjson_get_element_by_path(zjson_t *j, const char *path);

/* 已知json {group:{linux:[{}, {}, {me: {age:18, sex:"male"}}}}, 则
 * zjson_get_element_by_path_vec(json, "group", "linux", "2", "me", 0); 返回的 应该是 {age:18, sex:"male"} */
zjson_t *zjson_get_element_by_path_vec(zjson_t *j, const char *path0, ...);
```
