
## 不定长字符串封装, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 支持不定长字符串,
其 STRUCT 类型是 **zbuf_t**, 是 [LIB-ZC](./README.md) 的基本数据结构

## 数据结构

```
struct zbuf_t {
    char *data;      /* 数据 */
    int len:31;      /* 长度 */
    unsigned int static_mode:1;
    int size:31;     /* 容量 */
    unsigned int unused_flag1:1;
}; 
```

## 函数: 基本操作

### zbuf_t *zbuf_create(int size);

* 创建
* 初始容量为 size
* 如果 size &lt; 0, 则取默认值

### void zbuf_free(zbuf_t *bf);

* 释放

### char *zbuf_data(const zbuf_t *bf);

* 数据 char *

### int  zbuf_len(const zbuf_t *bf);

* 长度

### void zbuf_put(zbuf_t *bf, int ch);

* 追加字节 ch 到尾部, 并返回 ch, 失败则返回 -1

### zbuf_reset(zbuf_t *bf);

* 重置为空字符串

### void zbuf_terminate(zbuf_t *bf);

* 结尾置 0

### void zbuf_truncate(zbuf_t *bf, int new_len);

* 截短字符串为新长度 new_len
* 如果 new_len 大于实际长度, 则不操作

### int zbuf_need_space(zbuf_t *bf, int need);

* 调整容量, 使剩余容量大于 need, 返回实际剩余容量

## 函数: 字符串


* 这几个函数, 和标准 C 库类似, 顾名思义即可

### void zbuf_strncpy(zbuf_t *bf, const char *src, int len);

* 类似 strncpy

### void zbuf_strcpy(zbuf_t *bf, const char *src);

* 类似 strcpy

### void zbuf_strncat(zbuf_t *bf, const char *src, int len);

* 类似 strncat

### void zbuf_strcat(zbuf_t *bf, const char *src);<BR />void zbuf_puts(zbuf_t *bf, const char *src);

* 类似 strcat

### void zbuf_memcpy(zbuf_t *bf, const void *mem, int len);

* 类似 memcpy

### void zbuf_memcat(zbuf_t *bf, const void *mem, int len);

* 把 mem 复制追加到 bf

### void zbuf_append(zbuf_t *bf, zbuf_t *bf2);

* 把 bf2 复制追加到 bf

## 函数: 格式化复制

### void zbuf_printf_1024(zbuf_t *bf, const char *format, ...);

* 类似 snprintf(some_ptr, 1024, format, ....)
* 逻辑上可以这么理解:

```
char buf[1024+1];
snprintf(buf, 1024, format, ...);
zbuf_strcat(bf, buf);  
```

## 函数: trim

### void zbuf_trim_right_rn(zbuf_t *bf);

* 删除(trim)最右侧的 **\r** **\n**

## 例子

```
#include "zc.h"

int main(int argc, char **argv)
{
    zbuf_t *bf = zbuf_create(0);

    zbuf_put(bf, 'A');

    zbuf_strcat(bf, "aaaaaaaaaaa");

    zbuf_put(bf, 'A');

    zbuf_printf_1024(bf, "nnnew_%d_fff", 123);

    printf("%d, %s\n", zbuf_len(bf), zbuf_data(bf));

    zbuf_free(bf);

    return 0;
}
```

## 栈版本 zbuf_t

### ZSTACK_BUF(name, size);

* 宏, 在栈上定义一个**固定容量**为 size 的 zbuf_t, 其指针为 name

### 例子

```
void foo(const char *p)
{
    ZSTACK_BUF(bf, 1024);
    
    zbuf_strcpy(bf, p);

    printf("%s\n", zbuf_data(bf));

    /* 不必释放 bf */
}
```

