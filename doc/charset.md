# 字符串编码识别

[LIB-ZC](https://gitee.com/linuxmail/lib-zc)是一个C扩展库,支持字符键探测识别

### 问题: 下面字符串的字符集编码是什么 ?
```
const char *s="\xb9\xe3\xb6\xab\xca\xa1";
```

### 回答: 至少有4种字符集编码是合法的,  见下图
![在这里插入图片描述](https://img-blog.csdnimg.cn/20190617144340849.jpg?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L2VsaTk2MA==,size_16,color_FFFFFF,t_70)

从经验看, 字符集应该是**GB18030**


### 字符集编码探测方法
**LIB-ZC** 是一个C语言扩展库. 源码:  [https://gitee.com/linuxmail/lib-zc](https://gitee.com/linuxmail/lib-zc)
支持字符集编码探测, 例子: [https://gitee.com/linuxmail/lib-zc/blob/master/sample/charset/detact.c](https://gitee.com/linuxmail/lib-zc/blob/master/sample/charset/detact.c)

代码片段如下
```
const char *s="\xb9\xe3\xb6\xab\xca\xa1";
int len = strlen(s);

zbuf_t *charset = zbuf_create(0);

if (zcharset_detect_cjk(s,  len, charset) == 0) {
			printf("%s\n", "not found, maybe ASCII");
} else {
			printf("%s\n", zbuf_data(charset));
}
```

核心函数
```
/* 语言是CJK, 再使用此函数 */
/* 失败: 返回 NULL */
/* 成功: 字节集编码保存在 charset_result */
char *zcharset_detect_cjk(const char *data, int size, zbuf_t *charset_result);
```
