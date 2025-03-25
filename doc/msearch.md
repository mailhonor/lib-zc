
[C++版本](./msearch_cpp.md)

## 多关键字匹配, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 内嵌多关键字搜索模块

文件格式和实现, 参考:  ../blob/master/src/search/msearch.c

## 数据结构

```
struct zmsearch_t {
    /* 隐藏细节, 不必深究 */
};
```

## 函数

### zmsearch_t *zmsearch_create();

* 创建搜索器

### void zmsearch_free(zmsearch_t *ms);

* 释放

### void zmsearch_add_token(zmsearch_t *ms, const char *word, int len);

* 增加条目

### void zmsearch_add_over(zmsearch_t *ms);

* 增加条目完毕, 之后不能再执行 zmsearch_add_token

### int zmsearch_match(zmsearch_t *ms, const char *str, int len, const char **matched_ptr, int *matched_len);

* 在长度为 len 的 str 中做多关键字搜索
* 返回 &gt; 0: 匹配成功
    * *matched_ptr 保存匹配的子字符串指针
    * *matched_len 保存匹配的子字符串长度
* 返回 0: 没搜索到
* 返回 &lt; 0: 系统错误

### int zmsearch_add_token_from_pathname(zmsearch_t *ms, const char *pathname);

* 从文件 pathname 中, 逐行执行 zmsearch_add_token
* 忽略空行
* 忽略行首为 ### 的行
* trim 掉行首尾的空字符

### zmsearch_t *zmsearch_create_from_data(const void *data);

* 把数据 data 映射为 zmsearch_t
* 返回 0: 失败
* 不可以对此 zmsearch_t 执行 zmsearch_add_*

### zmsearch_t *zmsearch_create_from_pathname(const char *pathname);

* 从文件 pathname 映射为 zmsearch_t
* 返回 0: 失败
* 不可以对此 zmsearch_t 执行 zmsearch_add_*

### const void *zmsearch_get_compiled_data(zmsearch_t *ms);

* 获取编译后的数据

### int zmsearch_get_compiled_len(zmsearch_t *ms);

* 获取编译后的数据长度

### int zmsearch_build(zmsearch_t *ms, const char *dest_db_pathname);

* 编译, 把数据映射到文件 dest_db_pathname
* 返回 1 成功

## 搜索 1

### 第一步, 创建一个搜索器

```
zmsearch_t *ms = zmsearch_create();
```

### 第二步, 添加关键字

```
zmsearch_add_token(ms, "abc", 3);
zmsearch_add_token(ms, "someKey", -1);
zmsearch_add_token_from_pathname(ms, "keyword_list.txt");
zmsearch_add_token_from_pathname(ms, "keyword_list2.txt");
zmsearch_add_token(ms, "anotherKKK", -1);
```

### 第三步, 添加关键字完毕

```
zmsearch_add_over(ms);
```

### 第四步, 搜索

```
const char *str = "ABcXYZanotherKKKFSFSF";
const char *result;
int len;
int ok = zmsearch_match(ms, str, -1, &result, &len);
/* 应该是匹配成功了 "anotherKKK" */
/*    *result =  str + 6; */
/*     len = strlen("anotherKKK"); */
```

### 第五步, 释放资源

```
zmsearch_free(ms);
```

## 搜索 2

### 第一步, 从文件映射一个搜索器

```
zmsearch_t *ms = zmsearch_create_from_pathname("some_msearch_mmap_file.db");
```

### 第二步, 搜索

```
/* (同搜索1) */

```

### 第三步, 释放资源

```
zmsearch_free(ms);
```

## 编译: 得到映射(序列化)数据

### 第一步, 创建一个搜索器

```
zmsearch_t *ms = zmsearch_create();
```

### 第二步, 添加关键字

```
zmsearch_add_token(ms, "abc", 3);
zmsearch_add_token(ms, "someKey", -1);
zmsearch_add_token_from_pathname(ms, "keyword_list.txt");
zmsearch_add_token_from_pathname(ms, "keyword_list2.txt");
zmsearch_add_token(ms, "anotherKKK", -1);
```

### 第三步, 添加关键字完毕

```zmsearch_add_over(ms);```

### 第四步 1, 获取映射数据, 保存到文件

```
void *data = zmsearch_get_compiled_data(ms);
int dlen = zmsearch_get_compiled_data(ms);
fwrite(data, 1, dlen, some_fp);
```

### 第四步 2, 保存到文件

```
zmsearch_build(ms, "some_new_keyword.db");
```

### 第六步, 释放资源
```
zmsearch_free(ms);
```

## 例子

* ../blob/master/sample/search/msearch_match.c
* ../blob/master/sample/search/msearch_builder.c

