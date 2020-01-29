# 配置, config

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

**zconfig_t** 是配置, 是  **zdict_t**的别名, 是LIB-ZC基本数据结构

推荐使用, 不强制使用. 一些内嵌服务如master/server使用此配置风格  

## 配置文件

一个简单的通用配置文件风格
 * 行首第一个非空字符是#, 则忽略本行
 * 每配置行以 "=" 为分隔符
 * 配置项和配置值都需要过滤掉两侧的空白
 * 不支持任何转义
 * 相同配置项, 以后一个为准

## 继承自zdict_t 的函数

|  |  |  |
|:--|:--|:--|
| #define | zconfig_create | zdict_create |
| #define | zconfig_free | zdict_free |
| #define | zconfig_update | zdict_update |
| #define | zconfig_update_string | zdict_update_string |
| #define | zconfig_delete | zdict_delete |
| #define | zconfig_debug_show | zdict_debug_show |
| #define | zconfig_second_table_t | zconfig_long_table_t |
| #define | zconfig_size_table_t | zconfig_long_table_t |
| #define | zconfig_get_str | zdict_get_str | 
| #define | zconfig_get_bool | zdict_get_bool |
| #define | zconfig_get_int | zdict_get_int | 
| #define | zconfig_get_long | zdict_get_long |
| #define | zconfig_get_second | zdict_get_second |
| #define | zconfig_get_size | zdict_get_size |
| #define | ZCONFIG_WALK_BEGIN| ZDICT_WALK_BEGIN |
| #define | ZCONFIG_WALK_END| ZDICT_WALK_END |

## 函数介绍

**加载配置文件**

```
/* 覆盖同名配置项*/
/* 返回值 <0: 失败  */
int zconfig_load_from_pathname(zconfig_t *cf, const char *pathname);
```

**从另一个配置加载**

```
/* 覆盖同名配置项*/
void zconfig_load_annother(zconfig_t *cf, zconfig_t *another);
```

## 加载大量配置

### 数据结构

```
typedef struct {
    const char *name;
    const char *defval;
    const char **target;
} zconfig_str_table_t;
typedef struct {
    const char *name;
    int defval;
    int *target;
    int min;
    int max;
} zconfig_int_table_t;
typedef struct {
    const char *name;
    long defval;
    long *target;
    long min;
    long max;
} zconfig_long_table_t;
typedef struct {
    const char *name;
    int defval;
    int *target;
} zconfig_bool_table_t;
#define zconfig_second_table_t  zconfig_long_table_t
#define zconfig_size_table_t    zconfig_long_table_t
```

### 函数

```
void zconfig_get_str_table(zconfig_t *cf, zconfig_str_table_t *table);
void zconfig_get_int_table(zconfig_t *cf, zconfig_int_table_t *table);
void zconfig_get_long_table(zconfig_t *cf, zconfig_long_table_t *table);
void zconfig_get_bool_table(zconfig_t *cf, zconfig_bool_table_t *table);
void zconfig_get_second_table(zconfig_t *cf, zconfig_second_table_t *table);
void zconfig_get_size_table(zconfig_t *cf, zconfig_size_table_t *table);
```

## 全局配置

LIB-ZC框架维护了一个全局配置

```
extern zconfig_t *zvar_default_config;
```

初始化

```
zconfig_t *zdefault_config_init(void);
```

LIB-ZC内默认不使用 zvar_default_config, 但一些内嵌的服务器框架使用.

## 例子: 加载配置文件

```
#include "zc.h"
int main(int argc, char **argv)
{
    int i;
    zconfig_t *config;

    if (argc == 1) {
            printf("USAGE: %s config_pathname\n", argv[0]);
 		   return 1;
    }
    config = zconfig_create();

    /* 加载多个配置文件 */
    for (i = 1; i < argc; i++) {
        char *fn = argv[i];
        if (zconfig_load_from_pathname(config, fn) < 0) {
            printf("ERROR: load %s\n", fn);
            exit(1);
        }
    }

    zconfig_update_string(config, "date", "2015-10-21", -1);
    zconfig_update_string(config, "author", "eli960@qq.com", -1);

    /* 输出配置 */
    zconfig_debug_show(config);

    zconfig_free(config);
    return 0;
}
```

## 例子: 处理大量配置

```
void foo(zconfig_t *config)
{
    const char *var_httpd_www_path;
    const char *var_httpd_upload_tmp_path;
    zconfig_str_table_t str_table[] = { 
        "httpd_www_path", "www/", &var_httpd_www_path,
        "httpd_upload_tmp_path", "www/tmp..upload/", &var_httpd_upload_tmp_path,
        0, 0, 0
    }; 
    zconfig_get_str_table(config, str_table);
    
    int var_test_mode;
    int var_httpd_no_cache;
    zconfig_bool_table_t bool_table[] = {
        "zytest_mode", 0, &var_test_mode,
        "httpd_no_cache", 0, &zvar_httpd_no_cache,
        0, 0, 0
    };
    zconfig_get_bool_table(config, bool_table);

    int var_httpd_upload_file_size_limit
    zconfig_size_table_t size_table[] = {
        "httpd_upload_file_size_limit", 0, &var_httpd_upload_file_size_limit, 0, 100*1024*1024,
        0, 0, 0, 0, 0
    };
    zconfig_get_size_table(config, size_table);
}
```
