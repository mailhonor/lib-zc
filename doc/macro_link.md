# 宏 , 数据结构 , 链表

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

提供一组宏, 可实现各类链表操作. 不涉及到内存分配

推荐使用, 每次都自己实现的话,一不注意就出错了.可单独使用

## 单词(参数)解释

head: 链表头, 指针

tail: 链表尾, 指针

node, before: 节点变量, 指针

prev: 名称, 结构体成员, 前一个

next: 名称, 结构体成员, 后一个

## 宏

*宏的实现见本文底部*

### 追加node到尾部 

```
#define ZMLINK_APPEND(head, tail, node, prev, next)
```

### 追加node到头部

```
#define ZMLINK_PREPEND(head, tail, node, prev, next)
```

### 插入node到before前

```
#define ZMLINK_ATTACH_BEFORE(head, tail, node, prev, next, before)
```

### 去掉节点node

```
#define ZMLINK_DETACH(head, tail, node, prev, next)
```

### 例子

```
#include "zc.h"

typedef struct TTT TTT;
struct TTT {
    TTT *prev_123;
    TTT *next_456;
    int a;
};

int main(int argc, char **argv)
{
    TTT *head_111 = 0;
    TTT *tail_222 = 0;

    TTT *n; 

    n = (TTT *)malloc(sizeof(TTT));   n->a = 1;
    ZMLINK_APPEND(head_111, tail_222, n, prev_123, next_456);

    n = (TTT *)malloc(sizeof(TTT));  n->a = 2;
    ZMLINK_APPEND(head_111, tail_222, n, prev_123, next_456);
    
    n = (TTT *)malloc(sizeof(TTT));  n->a = 8;
    ZMLINK_PREPEND(head_111, tail_222, n, prev_123, next_456);

    n = head_111;
    ZMLINK_DETACH(head_111, tail_222, n, prev_123, next_456);
    printf("%d\n", n->a);
    free(n);

    n = head_111;
    ZMLINK_DETACH(head_111, tail_222, n, prev_123, next_456);
    printf("%d\n", n->a);
    free(n);

    n = tail_222;
    ZMLINK_DETACH(head_111, tail_222, n, prev_123, next_456);
    printf("%d\n", n->a);
    free(n);

    return 0;
```
## 其他例子
https://gitee.com/linuxmail/lib-zc/blob/master/sample/stdlib/mlink.c
https://gitee.com/linuxmail/lib-zc/blob/master/src/stdlib/list.c

## 宏的定义
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
```
