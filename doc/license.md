# 一种简单的序列号

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

提供一种简单的基于网卡mac地址的序列号生成/检测方法

## 说明

下文所谓的 "**mac地址**", 大写,形如:

```
00:0C:AA:BA:DF:EC
```

所谓 "**序列号**",大写, 0-9A-F, 长度16, 形如

```
D1DDA02369DDE895
```

## 函数

### 生成序列号

```
/* salt: 盐 */
/* mac: mac地址 */
/* 生成一个序列号, 结果存储到 result */
void zlicense_mac_build(const char *salt, const char *mac, zbuf_t *result);
```

### 验证

```
/* salt: 盐 */
/* license: 序列号*/
/* 返回值: -1: 错误(获取mac地址失败), 0:不匹配, 1: 匹配 */
int zlicense_mac_check(const char *salt, const char *license);
```

## 例子

```
#include "zc.h"

int main(int argc, char **argv)
{
    char *salt = 0, *mac = 0,  *license = 0;
    zmain_argument_run(argc, argv, 0); 
    printf("%s -salt salt_string -mac mac_address      #generate license\n", zvar_progname);
    printf("%s -salt salt_string -license license      #check lincese\n", zvar_progname);

    salt = zconfig_get_str(zvar_default_config, "salt", 0); 
    mac = zconfig_get_str(zvar_default_config, "mac", 0); 
    license = zconfig_get_str(zvar_default_config, "license", 0); 

    if (zempty(mac)) {
        int ret = zlicense_mac_check(salt, license);
        if (ret == 1) {
            printf("OK\n");
        } else if (ret == 0) {
            printf("NO\n");
        } else if (ret < 0) {
            printf("ERR\n");
        } else {
            printf("UNKNOWN\n");
        }
    } else {
        zbuf_t *nlicense = zbuf_create(0);
        zlicense_mac_build(salt, mac, nlicense);
        printf("%s\n", zbuf_data(nlicense));
        zfree(nlicense);
    }
    return 0;
}
```
