# 不定长字符串封装

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库,

zbuf\_t 是一个C不定长字符串的封装, 是LIB-ZC基本数据结构.

## 函数介绍

**创建zbuf_t**

```
/* size: 初始化容量.  <0: 则由本库指定 */
zbuf_t *zbuf_create(int size);
```

**释放 zbuf_t**

```
void zbuf_free(zbuf_t *bf);
```

**追加写字节ch到bf. 返回ch**

```
void zbuf_put(zbuf_t *bf, int ch);
```

**重置**

```
zbuf_reset(zbuf_t *bf);
```

**结尾置 0**

```
void zbuf_terminate(zbuf_t *bf);
```

**截短**

```
/* 如果new_len大于实际长度则不操作 */
void zbuf_truncate(zbuf_t *bf, int new_len);
```

**复制字符串**

```
/* 这几个函数, 和标准C库类似, 顾名思义即可 */
void zbuf_strncpy(zbuf_t *bf, const char *src, int len);
void zbuf_strcpy(zbuf_t *bf, const char *src);
void zbuf_strncat(zbuf_t *bf, const char *src, int len);
void zbuf_strcat(zbuf_t *bf, const char *src);
#define zbuf_puts zbuf_strcat
void zbuf_memcpy(zbuf_t *bf, const void *src, int len);
void zbuf_memcat(zbuf_t *bf, const void *src, int len);
void zbuf_append(zbuf_t *bf, zbuf_t *bf2) ;
```

**printf**

```
void zbuf_printf_1024(zbuf_t *bf, const char *format, ...);
```
类似snprintf(some_ptr, 1024, format, ....);
```
char buf[1024+1];
snprintf(buf, 1024, format, ...);
zbuf_cat(bf, buf);  
```

**删除最右侧的 \r \n**

```
void zbuf_trim_right_rn(zbuf_t *bf);
```

**宏: 返回数据指针**

```
/* char * /  zbuf_data(bf); 
```

**宏: 返回长度**

```
/* int /  zbuf_len(bf); 
```

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
