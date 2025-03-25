
## cint and data, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 在内存传输的时候,
有一种对长度本身(int)的特殊编码, 可以节约一部分内存

所谓 **cint_and_data** 抄袭的 postfix 队列文件的存储方式

有一点绕, 不感兴趣的可以忽略本文

## 对 int 编码

原理, 可以参考源码: [goto](../blob/master/src/stdlib/cint_and_data.c)

下面给出一些例子, 整数和编码后的内存

<table class="tbview" width="100%">
<tr><td width="150">整数</td><td width="100"><nobr>编码后长度</nobr></td><td>编码后内存</td></tr>
<tr><td>1</td><td>1</td><td>81</td></tr>
<tr><td>9</td><td>1</td><td>89</td></tr>
<tr><td>127</td><td>1</td><td>FF</td></tr>
<tr><td>129</td><td>2</td><td>01 81</td></tr>
<tr><td>312</td><td>2</td><td>38 82</td></tr>
<tr><td>654321</td><td>3</td><td>71 77 A7</td></tr>
</table>

上述编码后的 int, 称为 **cint**

## 函数: 编码

### void zcint_data_escape(zbuf_t * zb, const void *data, int len);

* 把 cint(len)+data, 追加到 zb
* 称这个过程为: 对长度为 len 的 data 做**编码**

### void zcint_data_escape_int(zbuf_t * zb, int i);

* 已知 char buf[32]; sprintf(buf, "%d", i);
* 执行 zcint_data_escape(zb, buf, strlen(buf))

### void zcint_data_escape_long(zbuf_t * zb, long i);

* 参考 zcint_data_escape_int

### void zcint_data_escape_dict(zbuf_t * zb, zdict_t * zd);

* 遍历 zd, 把每对(键&lt;=&gt;值)编码并追加到 zb

### void zcint_data_escape_pp(zbuf_t * zb, const char **pp, int size);

* 从指针开始遍历长度为 size 的 pp, 把 (*pp) 编码并追加到 zb

### int zcint_put(int size, char *buf);

* 把 cint(size) 保存到buf, 返回编码后的长度

## 函数: 解码

### int zcint_data_unescape(const void *src_data, int src_size, void **result_data, int *result_len);

* src_data形如: cint(length(somedata)) + somedata + otherdatas
* 数据指针(somedata)存储到  *result_data
* 长度(lenght(somedata))存储到 *result_len
* 解码一次, 返回偏移(otherdatas)

### int zcint_data_unescape_all(const void *src_data, int src_size, zsize_data_t *vec, int vec_size);

* 解码最多 vec_size 组 cint+data
* 解开后, 数据指针/长度 存储到 vec
* 返回偏移

## 函数: zstream_t

* 参考: [zstream_t](./stream.md)

### int zcint_data_get_from_zstream(zstream_t *fp);

* 从 fp 读数据, 并解开 cint

### int zstream_get_cint(zstream_t *fp);

* 从 fp 读数据, 并解开 cint

### int zstream_write_cint(zstream_t *fp, int len);

* 把 cint(len) 写到 fp

### int zstream_write_cint_and_data(zstream_t *fp, const void *buf, int len);

意思:
```
zstream_write_cint(fp, len);
zstream_write(fp, buf, len);
```

### int zstream_write_cint_and_int(zstream_t *fp, int i);

意思:
```
char buf[32]; sprintf(buf, "%d", i);
zstream_write_cint_and_data(fp, buf, strlen(buf));
```

### int zstream_write_cint_and_long(zstream_t *fp, long l);

* 参考 zstream_write_cint

### int zstream_write_cint_and_dict(zstream_t *fp, zdict_t * zd);

* 遍历 zd, 把每对(键&lt;=&gt;值)编码写到fp

### int zstream_write_cint_and_pp(zstream_t *fp, const char **pp, int size);

* 从指针开始遍历长度为 size 的 pp, 把 (*pp) 编码后的结果写到fp

## 函数: zaio_t

参考: [zaio_t](./aio.md)

### void zaio_get_cint(zaio_t *aio, void (*callback)(zaio_t *aio));

* 请求读, 读取 **压缩的int**
* 成功/失败/超时后回调执行 callback 

### void zaio_get_cint_and_data(zaio_t *aio, void (*callback)(zaio_t *aio));

* 请求读, 读取 **压缩的int** 所指的数据
* 成功/失败/超时后回调执行 callback 

### void zaio_cache_write_cint(zaio_t *aio, int len);

* 向缓存写 **压缩的int**

### void zaio_cache_write_cint_and_data(zaio_t *aio, const void *data, int len);

* 向缓存写 **压缩的int** 和数据

