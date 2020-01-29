# 文件

文件操作. 源码见 src/stdlib/file.c

---


```
/* -1: 错, >=0: 文件大小 */
int zfile_get_size(const char *pathname);

/* 保存data 到文件pathname, 覆盖pathname -1: 错, 1: 成功 */
int zfile_put_contents(const char *pathname, const void *data, int len);

/* 从文件pathname获取文件内容, 存储到(覆盖)result, -1:错, >= 文件长度 */
int zfile_get_contents(const char *pathname, zbuf_t *result);
/* 同上, 出错exit */
int zfile_get_contents_sample(const char *pathname, zbuf_t *result);

/* 从标准输入读取内容到(覆盖)bf */
int zstdin_get_contents(zbuf_t *bf);

/* mmap reader */
struct zmmap_reader_t {
    int fd;
    int len; /* 映射后, 长度 */
    char *data; /* 映射后, 指针 */
};

/* mmap 只读映射一个文件, -1: 错, 1: 成功  */
int zmmap_reader_init(zmmap_reader_t *reader, const char *pathname);
int zmmap_reader_fini(zmmap_reader_t *reader);
```
