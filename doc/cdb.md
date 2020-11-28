<A name="readme_md" id="readme_md"></A>

## 一种新的静态 DB (CDB), [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 内嵌一种新的静态 DB (CDB),
不支持修改, 读取线程安全, 当前版本代码 "0001"

本文不介绍文件格式, 有兴趣的可以看源码 https://gitee.com/linuxmail/lib-zc/blob/master/src/cdb/

## 数据结构

```
struct zcdb_t {
    /* 阅读器; 隐藏细节, 不必深究 */
};
struct zcdb_walker_t {
    /* 遍历器; 隐藏细节, 不必深究 */
};
struct zcdb_builder_t {
    /* 生成器; 隐藏细节, 不必深究 */
};
```

## 函数: 阅读器

### zcdb_t *zcdb_open(const char *cdb_pathname);<BR />zcdb_t *zcdb_open2(const char *cdb_pathname, zbuf_t *error_msg);

* 打开cdb文件 cdb_pathname
* 返回 0: 失败, 失败原因存储到 error_msg

### void zcdb_close(zcdb_t *cdb);

* 释放

### int zcdb_get_count(zcdb_t *cdb);

* 成员个数

### int zcdb_find(zcdb_t *cdb, const void *key, int klen, void **val, int *vlen);

* 查找; 线程安全;
* 查找键为key(其长度:klen)的值, 结果地址保存到 *val, 长度保存到 *vlen
* 返回 -1:出错, 0: 没找到, 1: 找到

## 函数: 遍历器


### zcdb_walker_t *zcdb_walker_create(zcdb_t *cdb);

* 创建遍历器

### void zcdb_walker_free(zcdb_walker_t *walker);

* 释放

### int zcdb_walker_walk(zcdb_walker_t *walker, void **key, int *klen, void **val, int *vlen);

* 遍历
* 当前键的指针/长度存储到 *key/*klen; 值的指针/长度存储到 *val/*vlen;
* 返回 -1:出错, 0: 没找到(结尾), 1: 找到

### void zcdb_walker_reset(zcdb_walker_t *walker);

* 重置

## 函数: 生成器


### zcdb_builder_t *zcdb_builder_create();

* 创建生成器

### void zcdb_builder_free(zcdb_builder_t *builder);

* 释放

### void zcdb_builder_update(zcdb_builder_t *builder, const void *key, int klen, const void *val, int vlen);

* 添加或更新
* 键为 key/klen
* 值为 val/vlen 

### int zcdb_builder_build(zcdb_builder_t *builder, const char *dest_db_pathname);

* 生成 cdb 文件
* 保存(覆盖写)到文件 dest_db_pathname

## 例子

* https://gitee.com/linuxmail/lib-zc/blob/master/sample/cdb/

