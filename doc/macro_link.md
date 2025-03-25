
## 宏: 链表, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 提供一组宏,
支持双向链表, 可实现各类链表操作. 不涉及到内存分配

推荐使用, 每次都自己实现的话,一不注意就出错了, 可单独使用

## 单词(参数)解释

```
head:   链表头, 指针
tail:   链表尾, 指针
node:   节点变量, 指针
before: 节点变量, 指针
prev:   名称, 结构体成员, 前一个
next:   名称, 结构体成员, 后一个
```

## 宏: 说明

### ZMLINK_APPEND(head, tail, node, prev, next);

* 追加 node 到尾部 


### ZMLINK_PREPEND(head, tail, node, prev, next);

* 追加 node 到头部

### ZMLINK_ATTACH_BEFORE(head, tail, node, prev, next, before);

* 把 node 插入到 before 前

### ZMLINK_DETACH(head, tail, node, prev, next);

* 移除 node

### ZMLINK_CONCAT(head_1, tail_1, head_2, tail_2, prev, next);

* 合并链表, 把第 2 个链表的头(head_2) 追加到第 1 个链表的尾部(tail_1)

## 宏: 定义

```
#define ZMLINK_APPEND(head, tail, node, prev, next) {\
    typeof(head) _head_1106=head, _tail_1106=tail, _node_1106 = node;\
    if(_head_1106 == 0){_node_1106->prev=_node_1106->next=0;_head_1106=_tail_1106=_node_1106;}\
    else {_tail_1106->next=_node_1106;_node_1106->prev=_tail_1106;_node_1106->next=0;_tail_1106=_node_1106;}\
    head = _head_1106; tail = _tail_1106; \
}

#define ZMLINK_PREPEND(head, tail, node, prev, next) {\
    typeof(head) _head_1106=head, _tail_1106=tail, _node_1106 = node;\
    if(_head_1106 == 0){_node_1106->prev=_node_1106->next=0;_head_1106=_tail_1106=_node_1106;}\
    else {_head_1106->prev=_node_1106;_node_1106->next=_head_1106;_node_1106->prev=0;_head_1106=_node_1106;}\
    head = _head_1106; tail = _tail_1106; \
}

#define ZMLINK_ATTACH_BEFORE(head, tail, node, prev, next, before) {\
    typeof(head) _head_1106=head, _tail_1106=tail, _node_1106 = node, _before_1106 = before;\
    if(_head_1106 == 0){_node_1106->prev=_node_1106->next=0;_head_1106=_tail_1106=_node_1106;}\
    else if(_before_1106==0){_tail_1106->next=_node_1106;_node_1106->prev=_tail_1106;_node_1106->next=0;_tail_1106=_n
ode_1106;}\
    else if(_before_1106==_head_1106){_head_1106->prev=_node_1106;_node_1106->next=_head_1106;_node_1106->prev=0;_hea
d_1106=_node_1106;}\
    else {_node_1106->prev=_before_1106->prev; _node_1106->next=_before_1106; _before_1106->prev->next=_node_1106; _b
efore_1106->prev=_node_1106;}\
    head = _head_1106; tail = _tail_1106; \
}

#define ZMLINK_DETACH(head, tail, node, prev, next) {\
    typeof(head) _head_1106=head, _tail_1106=tail, _node_1106 = node;\
    if(_node_1106->prev){ _node_1106->prev->next=_node_1106->next; }else{ _head_1106=_node_1106->next; }\
    if(_node_1106->next){ _node_1106->next->prev=_node_1106->prev; }else{ _tail_1106=_node_1106->prev; }\
    head = _head_1106; tail = _tail_1106; \
}

#define ZMLINK_CONCAT(head_1, tail_1, head_2, tail_2, prev, next) {\
    typeof(head_1) _head_1106=head_1,_tail_1106=tail_1,_head_2206=head_2,_tail_2206=tail_2; if(_head_2206){ \
        if(_head_1106){_tail_1106->next=_head_2206;_head_2206->prev=_tail_1106; }else{_head_1106=_head_2206;} \
        _tail_1106=_tail_2206; \
    } head_1 = _head_1106; tail_1 = _tail_1106; \
}
```

## 例子

* [goto](../blob/master/sample/stdlib/mlink.c)
* [goto](../blob/master/sample/stdlib/mlink2.c)
* [goto](../blob/master/src/stdlib/list.c)

