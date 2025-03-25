
## 词典, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 支持词典,
其 STRUCT 类型是 **zdict_t**, 基于[红黑树(zrbtree_t)](./rbtree.md)实现,
是 [LIB-ZC](./README.md) 的基本数据结构

## 数据结构

```
struct zdict_t {
    zrbtree_t rbtree;
    int len;           /* 节点个数 */
};
struct zdict_node_t {
    zrbtree_node_t rbnode;
    zbuf_t value;      /* 值 */
    char *key;         /* 键 */
} __attribute__ ((aligned(8)));
```

## 函数: 基本操作

### zdict_t *zdict_create(void);

* 创建词典

### void zdict_free(zdict_t *dict);

* 释放

### void zdict_reset(zdict_t *dict);

* 重置 

### int zdict_len(zdict_t *dict);

* 宏, 节点个数

### void zdict_debug_show(const zdict_t *dict);

* 打印词典, 调试用

## 函数: 增加或覆盖

### zdict_node_t *zdict_update(zdict_t *dict, const char *key, const zbuf_t *value);

* 如果键为 key 的节点不存在, 则新增一个节点并返回, 此节点键为key, 值为 value 的拷贝
* 如果键为 key 的节点存在, 则修改此节点并返回, 修改值为 value 的拷贝

### zdict_node_t *zdict_update_string(zdict_t *dict, const char *key, const char *value, int len);

* 如果键为 key 的节点不存在, 则新增一个节点并返回, 此节点键为key, 值为 (len&lt;0?strdup(value):strndup(value, len))
* 如果键为 key 的节点存在, 则修改此节点并返回, 修改值为 (len&lt;0?strdup(value):strndup(value, len))

## 函数: 删除节点

### void zdict_delete(zdict_t *dict, const char *key);

* 删除键为 key 的节点

### void zdict_delete_node(zdict_t *dict, zdict_node_t *node);

* 删除节点 node

## 函数: 查找节点


### zdict_node_t *zdict_first(const zdict_t *dict);

* 第一个节点

### zdict_node_t *zdict_last(const zdict_t *dict);

* 最后一个节点

### zdict_node_t *zdict_find(const zdict_t *dict, const char *key, zbuf_t **value);

* 查找键为 key 的节点, 如果存在则返回此节点且节点的值赋值给 *value, 否则返回 0

### zdict_node_t *zdict_find_near_prev(const zdict_t *dict, const char *key, zbuf_t **value);

* 查找键小于或等于key的最大的节点, 如果存在则返回此节点且节点的值赋值给 *value, 否则返回 0

### zdict_node_t *zdict_find_near_next(const zdict_t *dict, const char *key, zbuf_t **value);

* 查找键大于或等于key的最小的节点, 如果存在则返回此节点且节点的值赋值给 *value, 否则返回 0

## 函数: 节点属性


### zdict_node_t *zdict_prev(const zdict_node_t *node);

* 前一个节点

### zdict_node_t *zdict_next(const zdict_node_t *node);

* 后一个节点

### char *zdict_node_key(zdict_node_t *node);

* 宏, 节点的键

### zbuf_t *zdict_node_value(zdict_node_t *node);

* 宏, 节点的值

## 函数, 遍历


### ZDICT_WALK_BEGIN(zdict_t *dict, var_your_key, var_your_value);<BR />ZDICT_WALK_END;

* 宏, 遍历

### ZDICT_NODE_WALK_BEGIN(zdict_t *dict, var_your_node);<BR />ZDICT_NODE_WALK_END;

* 宏, 遍历

## 函数: 查找(扩展)

_本节提到的 zstr_to_bool, zstr_to_second, zstr_to_size, 请参考 [字符串函数](./string.md)_

### char *zdict_get_str(const zdict_t *dict, const char *name, const char *def);

* 查找键为 name 的节点, 如果存在则返回其值(string), 否则返回 def

### int zdict_get_bool(const zdict_t *dict, const char *name, int def);

* 查找键为 name 的节点, 如果存在则返回 zstr_to_bool(其值), 否则返回 def

### int zdict_get_int(const zdict_t *dict, const char *name, int def);

* 查找键为 name 的节点, 如果存在则返回 foo(其值), 否则则返回 foo(其值), 否则返回def
* foo 为 atoi

### long zdict_get_long(const zdict_t *dict, const char *name, long def);

* 如上, foo 为 atol

### long zdict_get_second(const zdict_t *dict, const char *name, long def);

* 如上, foo 为 zstr_to_second, 支持 s/m/h/d/w, s(second), m(minute), h(hour), d(day), w(week)
* 如: 19m =&gt; 19*60, 2w =&gt; 2*7*24*3600 

### long zdict_get_size(const zdict_t *dict, const char *name, long def);

* 如上, foo 为 zstr_to_size, 支持 b/k/m/g, b(byte), k(1024), m(1024*1024), g(1024*1024*1024)
* 如: 128K =&gt; 128*1024, 8M =&gt; 8*1024*1024

## 例子

* [goto](../blob/master/sample/rbtree/dict_demo.c)

