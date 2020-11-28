<A name="readme_md" id="readme_md"></A>

## map 映射, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 支持 map 映射,
其 STRUCT 类型是 **zmap_t**, 基于 [红黑树(zrbtree_t)](./rbtree.md) 实现,
是 [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 的基本数据结构

## 数据结构

```
struct zmap_t {
    zrbtree_t rbtree;
    int len;
};
struct zmap_node_t {
    char *key;   /* 键 */ 
    void *value; /* 值 */
    zrbtree_node_t rbnode;
} __attribute__ ((aligned(8)));
```

## 函数: 基本操作

### zmap_t *zmap_create(void);

* 创建

### void zmap_free(zmap_t *map);

* 释放

### void zmap_reset(zmap_t *map);

* 重置

### int zmap_len(zmap_t *map);

* 宏,节点个数

## 函数: 增加

### zmap_node_t *zmap_update(zmap_t *map, const char *key, const void *value, void **old_value);

* 如果键为 key 的节点不存在, 则新增一个节点并返回, 此节点键为key, 值为 value, 同时设置 *old_value 为 0
* 如果键为 key 的节点存在, 则更新此节点并返回, 修改值为 value, 同时设置 *old_value 为 旧值

## 函数: 删除节点

### zbool_t zmap_delete(zmap_t * map, const char *key, void **old_value);

* 删除键为 key 的节点, 如果存在则返回 1, 否则返回 0, 如果存在则节点的值赋值给 *old_value

### void zmap_delete_node(zmap_t *map, zmap_node_t *node, void **old_value);

* 删除键为 key 节点 node, 节点 node 的值赋值给 *old_value

## 函数: 查找节点

### zmap_node_t *zmap_first(const zmap_t *map);

* 第一个节点

### zmap_node_t *zmap_last(const zmap_t *map);

* 最后一个节点

### zmap_node_t *zmap_find(const zmap_t *map, const char *key, void **value);

* 查找键为 key 的节点, 如果存在则返回此节点且节点的值赋值给 *value, 否则返回 0

### zmap_node_t *zmap_find_near_prev(const zmap_t *map, const char *key, void **value);

* 查找键小于或等于key的最大的节点, 如果存在则返回此节点且节点的值赋值给 *value, 否则返回 0

### zmap_node_t *zmap_find_near_next(const zmap_t *map, const char *key, void **value);

* 查找键大于或等于key的最小的节点, 如果存在则返回此节点且节点的值赋值给 *value, 否则返回 0

## 函数: 节点属性

### zmap_node_t *zmap_prev(const zmap_node_t *node);

* 前一个节点

### zmap_node_t *zmap_next(const zmap_node_t *node);

* 后一个节点

### char *zmap_node_key(zmap_node_t *node);

* 宏, 节点的键

### void *zmap_node_value(zmap_node_t *node);

* 宏, 节点的值


## 函数, 遍历

### ZMAP_NODE_WALK_BEGIN(zmap_t *map, var_your_node);<BR />ZMAP_NODE_WALK_END

* 宏, 遍历

### ZMAP_WALK_BEGIN(zmap_t *map, var_your_key, var_your_value_type, var_your_value)<BR />ZMAP_WALK_END

* 宏, 遍历

## 例子

* https://gitee.com/linuxmail/lib-zc/blob/master/sample/rbtree/map_demo.c

