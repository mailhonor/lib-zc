<A name="readme_md" id="readme_md"></A>

## 数据结构: 链表, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 支持双向链表(数据结构),
其 STRUCT 类型是 **zlink_t**,
是 [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 的基本数据结构

所谓"**链表**"是一个数据结构, 通用链表算法. **不涉及到<s>内存分配</s>**

## 数据结构

```
/* 链表 */
struct zlink_t {
    zlink_node_t *head;
    zlink_node_t *tail;
};
/* 节点 */
struct zlink_node_t {
    zlink_node_t *prev;
    zlink_node_t *next;
};
```

## 函数: 基本操作

### void zlink_init(zlink_t *link);

* 初始化 link 指向的地址

### void zlink_fini(zlink_t *link);

* 反初始化 link 指向的地址

## 函数: 添加节点

### zlink_node_t *zlink_attach_before(zlink_t *link, zlink_node_t *node, zlink_node_t *before);

* 把 node 插到 before 前

### zlink_node_t *zlink_push(zlink_t *link, zlink_node_t *node);

* 把节点 node 追加到尾部

### zlink_node_t *zlink_unshift(zlink_t *link, zlink_node_t *node);

* 把节点 node 追加到首部 


## 函数: 移除节点

### zlink_node_t *zlink_detach(zlink_t *link, zlink_node_t *node);

* 移除节点 node, 不释放 node, 并返回

### zlink_node_t *zlink_pop(zlink_t *link);

* 移除尾部节点, 不释放这个节点, 并返回 

### zlink_node_t *zlink_shift(zlink_t * link);

* 移除首部节点, 不释放这个节点, 并返回 


## 函数, 特殊节点

### zlink_node_t *define zlink_head(zlink_t *link)

* 宏, 首部节点

### zlink_node_t *define zlink_tail(zlink_t *link)
* 宏, 尾部节点

## 节点属性

### zlink_node_t *zlink_node_prev(zlink_node_t *node);

* 宏, 前一个节点

### zlink_node_t *zlink_node_next(zlink_node_t *node);

* 宏, 后一个节点

## 例子: 1

实现一个简单的列表, 功能很简单:

* 表尾追加, 表头移除
* https://gitee.com/linuxmail/lib-zc/blob/master/sample/stdlib/link.c

## 例子: 2

* [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 的 [zlist_t](./list.md)是基于 zlink_t 实现的
* https://gitee.com/linuxmail/lib-zc/blob/master/src/stdlib/list.c

