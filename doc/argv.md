
## 字符串向量(argv), [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 支持字符串向量(argv),
其 STRUCT 类型是 **zargv_t**, 是 [LIB-ZC](./README.md) 的基本数据结构

## 数据结构

```
struct zargv_t {
    char **argv; /* 数据 */
    int argc:31; /* 个数 */
    int unused:1;
    int size:31; /* 容量 */
    int mpool_used:1;
};
```

## 函数: 基本操作

### zargv_t *zargv_create(int size);

* 创建; 初始容量为 size, 如果 size &lt; 0, 则取默认值

### void zargv_free(zargv_t *ar);

* 释放

### int zargv_len(zargv_t *ar);

* 宏, 个数

### int zargv_argc(zargv_t *ar);

* 个数

### char **zargv_data(zargv_t *ar);

* 宏, 数据指针

### char **zargv_argv(zargv_t *ar);

* 数据指针

### void zargv_debug_show(zargv_t *ar);

* 打印字符串向量, 调试用

## 函数: 增加(追加)字符串

### void zargv_add(zargv_t *ar, const char *ns);

* 把 strdup(ns) 追加到尾部

### void zargv_addn(zargv_t *ar, const char *ns, int nlen);

* 把 strndup(ns, nlen) 追加到为尾部

### zargv_t *zargv_split_append(zargv_t *ar, const char *string, const char *delim);

* 用 delim 分割 string, 把结果追加到尾部


## 函数: 截短

### void zargv_truncate(zargv_t *ar, int len);

* 截短为新长度(个数)len; 如果 (len &gt; zargv_len(ar)), 则无效


## 函数: 遍历

### ZARGV_WALK_BEGIN(zargv_t *ar, var_your_ptr);<BR />ZARGV_WALK_END;

* 宏, 遍历

## 例子

* ../blob/master/sample/stdlib/argv.c

