<A name="readme_md" id="readme_md"></A>

## 一种简单的序列号, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 内嵌一种简单的序列号生成和验证的方法,
这个方法基于 MAC 地址

_MAC 地址的获取参考: [int zget_mac_address(zargv_t *mac_list)](./dns.md)_

## 说明

所谓的 **MAC 地址**, 大写,形如:

```
00:0C:AA:BA:DF:EC
```

所谓 **序列号**,大写, 0-9A-F, 长度16, 形如

```
D1DDA02369DDE895
```

## 生成序列号

### void zlicense_mac_build(const char *salt, const char *mac, zbuf_t *result);

* 生成序列号
* salt: 盐
* mac: MAC 地址
* 生成的序列号存储(追加)到 result

## 验证序列号

### int zlicense_mac_check(const char *salt, const char *license);

* 验证序列号
* salt: 盐
* license: 序列号
* 返回 -1: 错误(获取 MAC 地址失败)
* 返回 0: 不匹配
* 返回 1: 匹配

## 例子

* https://gitee.com/linuxmail/lib-zc/blob/master/sample/stdlib/license.c

