# vector

## 简介

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

zvector_t 是 VECTOR实现

PS:

推荐使用 zvector_push, zvector_pop

不推荐使用 zvector_unshift, zvector_shift, zvector_insert, zvector_delete

## 数据结构

```
struct zvector_t {
    char **data; /* 数据 */
    int len;  /* 个数 */
    int size;  /* 容量 */
    int offset:31;
    unsigned int mpool_used:1;
};
```

## 函数

### 创建/释放

```
zvector_t *zvector_create(int size);
void zvector_free(zvector_t *v);
```

### 插入

```
/*  追加指针val到尾部 */
void zvector_push(zvector_t *v, const void *val);
#define zvector_add zvector_push

/* 追加指针val到头部 */
void zvector_unshift(zvector_t *v, const void *val);

/* 把指针val插到idx处, 原idx及其后元素顺序后移 */
void zvector_insert(zvector_t *v, int idx, void *val);
```

### 弹出

```
/* 弹出尾部指针, 并赋值给*val, 存在则返回1, 否则返回 */
zbool_t zvector_pop(zvector_t *v, void **val);

/* 弹出首部指针并赋值给*val, 存在则返回1, 否则返回 0 */
zbool_t zvector_shift(zvector_t *v, void **val);

/* 弹出idx处的指针并赋值给*val, 存在则返回1, 否则返回 0 */
zbool_t zvector_delete(zvector_t *v, int idx, void **val);
```

### 重置

```
/* 重置 */
void zvector_reset(zvector_t *v); 

/* 截短到 new_len */
void zvector_truncate(zvector_t *v, int new_len);
```

### 遍历宏

```
#define ZVECTOR_WALK_BEGIN(arr, you_chp_type, var_your_ptr)
#define ZVECTOR_WALK_END
```

## 例子

```
#include "zc.h"

int main(int argc, char **argv)
{
    zvector_t *vec = zvector_create(-1);
    int i;

    /* 插入/追加 */
    for (i = 0; i < 1000; i++) {
        zvector_push(vec, 0); 
    }   

    /* 头部弹出 */
    for (i = 0; i < 100; i++) {
        zvector_shift(vec, 0); 
    }   
    
    /* 插入/头部 */
    for (i = 0; i < 1000; i++) {
        zvector_unshift(vec, 0); 
    }   

    /* 弹出,尾部 */
    for (i = 0; i < 100; i++) {
        zvector_pop(vec, 0); 
    }   

    /* 插入 , 在idx是100000的地方得救 */
    zvector_insert(vec, 10000, 0); 

    while(zvector_len(vec) > 1) {
        zvector_delete(vec, 1, 0);
    }
    zvector_delete(vec, 0, 0);

    zvector_free(vec);
    return 0;
}
```
