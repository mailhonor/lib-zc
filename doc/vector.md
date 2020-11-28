<A name="readme_md" id="readme_md"></A>

## 向量(vector), [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 支持向量(vector),
其 STRUCT 类型是 **zvector_t**,
是 [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 的基本数据结构

推荐使用 zvector_push, zvector_pop;

不推荐使用 zvector_unshift, zvector_shift, zvector_insert, zvector_delete

## 数据结构

```
struct zvector_t {
    char **data;   /* 数据 */
    int len;       /* 个数 */
    int size;      /* 容量 */
    int offset:31;
    unsigned int mpool_used:1;
};
```

## 函数: 基本操作

### zvector_t *zvector_create(int size);

* 创建
* 初始容量为 size
* 如果 size &lt; 0, 则取默认值

### void zvector_free(zvector_t *v);

* 释放

### void zvector_reset(zvector_t *v);

* 重置

### int zvector_len(zvector_t *v);

* 个数

## 函数: 插入

### void zvector_add(zvector_t *v, const void *val);<BR />void zvector_push(zvector_t *v, const void *val);

* 追加 val 到尾部

### void zvector_unshift(zvector_t *v, const void *val);

* 追加 val 到头部

### void zvector_insert(zvector_t *v, int idx, void *val);

* 如果(idx+1 &lt;= 长度(v)), 则把 val 插到 idx 处, 原idx及其后元素顺序后移
* 如果(idx+1 &gt; 长度(v)), 则增加长度, 把 val 插到 idx 处, 其余位置设置为 0

## 函数: 移除

### zbool_t zvector_pop(zvector_t *v, void **val);

* 弹出尾部指针, 并赋值给 *val
* 存在则返回1, 否则返回 0

### zbool_t zvector_shift(zvector_t *v, void **val);

* 弹出首部指针, 并赋值给 *val
* 存在则返回1, 否则返回 0 

### zbool_t zvector_delete(zvector_t *v, int idx, void **val);

* 弹出idx处的指针并赋值给 *val
* 存在则返回1, 否则返回 0
* 同时, idx 后的元素顺序前移

### void zvector_truncate(zvector_t *v, int new_len);

* 截短为新长度 new_len
* 如果 new_len &gt; 长度(v), 则无效

## 函数: 遍历

### ZVECTOR_WALK_BEGIN(zvector_t *v, you_chp_type, var_your_ptr);<BR />ZVECTOR_WALK_END

* 宏, 遍历

## 例子

* https://gitee.com/linuxmail/lib-zc/blob/master/sample/stdlib/vector.c

