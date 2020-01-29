# 字符串列表 argv

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

支持字符串列表, 封装为 zargv_t, 是LIB-ZC的基本数据结构

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

## 函数(宏)

**个数**

```
#define zargv_len(ar)           ((ar)->argc)
#define zargv_argc(ar)          ((ar)->argc)
```

 <B>数据 char **</B>

```
#define zargv_data(ar)          ((ar)->argv)
```

**创建**

```
/* size: 初始容量 */
zargv_t *zargv_create(int size);
void zargv_free(zargv_t *argvp);
```

**增加(追加)字符串**

```
/* 把strdup(ns)追加到尾部*/
void zargv_add(zargv_t *argvp, const char *ns);

/* 把strndup(ns, nlen)追加到为尾部 */
void zargv_addn(zargv_t *argvp, const char *ns, int nlen);

/* 用delim分割string, 并追加到argvp */
zargv_t *zargv_split_append(zargv_t *argvp, const char *string, const char *delim);
```

**截短**

```
void zargv_truncate(zargv_t *argvp, int len);
```

**重置**

```
#define zargv_reset(ar)          (zargv_truncate((ar), 0))
```

**debug 输出**

```
void zargv_debug_show(zargv_t *argvp);
```

**遍历**

```
#define ZARGV_WALK_BEGIN(argvp, var_your_ptr)            ...
#define ZARGV_WALK_END                                           ...
```

## 例子

```
#include "zc.h"

static void test_general(void)
{
    zargv_t *ar;
    ar = zargv_create(1);
    zargv_add(ar, "aaa");
    zargv_addn(ar, "bbb", 8);
    zargv_addn(ar, "ccccccc", 3);
    zargv_split_append(ar, "this is good", "iso");
    zargv_debug_show(ar);
    zargv_free(ar);
}

static void test_split(void)
{
    zargv_t *ar;
    const char *original = "this is a test sentence.";

    zinfo("==========");
    ar = zargv_create(1);
    zargv_split_append(ar, original, " ");
    zargv_debug_show(ar);
    zargv_free(ar);

    zinfo("==========");
    ar = zargv_create(1);
    zargv_split_append(ar, original, "et");
    zargv_debug_show(ar);
    zargv_free(ar);

    zinfo("==========");
    ar = zargv_create(1);
    zargv_split_append(ar, original, "XXX");
    zargv_debug_show(ar);
    zargv_free(ar);

    zinfo("==========");
    ar = zargv_create(1);
    zargv_split_append(ar, "", "et");
    zargv_debug_show(ar);
    printf("count:%d\n", zargv_len(ar));
    zargv_free(ar);
}

int main(int argc, char **argv)
{
    test_general();
    test_split();

    return 0;
}
```
