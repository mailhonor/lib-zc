<A name="readme_md" id="readme_md"></A>

## 文件操作, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 封装了最常见的文件操作

## 文件读写

### int zfile_get_size(const char *pathname);

* 返回文件大小: &gt;= 0

### int zfile_put_contents(const char *pathname, const void *data, int len);

* 保存长度为 len 的 data 到文件 pathname, 覆盖 pathname
* 返回 1: 成功

### int zfile_get_contents(const char *pathname, zbuf_t *result);

* 从文件 pathname 获取文件内容, 存储到(覆盖)result
* 返回 &gt;=0: 文件长度, 成功

### int zfile_get_contents_sample(const char *pathname, zbuf_t *result);

* 同上; 出错exit(1); 用于写简单的测试代码

### int zstdin_get_contents(zbuf_t *result);

* 从标准输入读取内容, 存储到(覆盖)result
* 返回 &gt;=0: 文件长度, 成功

## mmap 读

```
struct zmmap_reader_t {
    int fd;
    int len;    /* 映射后, 长度 */
    char *data; /* 映射后, 指针, const char * */
};
```

### int zmmap_reader_init(zmmap_reader_t *reader, const char *pathname);

* 初始化
* mmap 只读(共享)映射一个文件
* 返回 1: 成功

### int zmmap_reader_fini(zmmap_reader_t *reader);

* 反初始化

