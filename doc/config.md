<A name="readme_md" id="readme_md"></A>

## key = value 型通用配置, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 支持通用配置,
其 STRUCT 类型是 **zconfig_t**,
是 [dict_t](./dict.md) 的别名,
是 [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 的基本数据结构

推荐使用, 一些内嵌服务如master/server使用此配置风格  

## 一般配置文件

```
# 忽略空行

# 行首第一个非空字符是#, 则忽略本行
     #
# 每配置行以 "=" 为分隔符
# 配置项和配置值都需要过滤掉两侧的空白
# 不支持任何转义
server-command = bin/milter
server-log = syslog,mail

# 相同配置项, 以后一个为准
spamd_server = var/socket/spamd
spamd_server = var/socket/spamd222
```

## 函数

### 继承自 zdict_t 的函数

请参考对应的 [zdict_t](./dict.md) 函数
```
#define    zconfig_create            zdict_create    
#define    zconfig_free              zdict_free    
#define    zconfig_update            zdict_update    
#define    zconfig_update_string     zdict_update_string    
#define    zconfig_delete            zdict_delete    
#define    zconfig_debug_show        zdict_debug_show    
#define    zconfig_second_table_t    zconfig_long_table_t    
#define    zconfig_size_table_t      zconfig_long_table_t    
#define    zconfig_get_str           zdict_get_str    
#define    zconfig_get_bool          zdict_get_bool    
#define    zconfig_get_int           zdict_get_int    
#define    zconfig_get_long          zdict_get_long    
#define    zconfig_get_second        zdict_get_second    
#define    zconfig_get_size          zdict_get_size    
#define    ZCONFIG_WALK_BEGIN        ZDICT_WALK_BEGIN    
#define    ZCONFIG_WALK_END          ZDICT_WALK_END    
```

### 加载配置文件

```
/* 覆盖同名配置项 */
/* 返回值 < 0: 失败  */
int zconfig_load_from_pathname(zconfig_t *cf, const char *pathname);
```

### 从另一个配置加载

```
/* 覆盖同名配置项*/
void zconfig_load_annother(zconfig_t *cf, zconfig_t *another);
```

### 大量配置: 数据结构

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
} zconfig_int_table_t;
typedef struct {
    const char *name;
    long defval;
    long *target;
} zconfig_long_table_t;
typedef struct {
    const char *name;
    int defval;
    int *target;
} zconfig_bool_table_t;

#define zconfig_second_table_t  zconfig_long_table_t
#define zconfig_size_table_t    zconfig_long_table_t
```

### 大量配置: 函数

```
void zconfig_get_str_table(zconfig_t *cf, zconfig_str_table_t *table);
void zconfig_get_int_table(zconfig_t *cf, zconfig_int_table_t *table);
void zconfig_get_long_table(zconfig_t *cf, zconfig_long_table_t *table);
void zconfig_get_bool_table(zconfig_t *cf, zconfig_bool_table_t *table);
void zconfig_get_second_table(zconfig_t *cf, zconfig_second_table_t *table);
void zconfig_get_size_table(zconfig_t *cf, zconfig_size_table_t *table);
```

## 全局配置

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 框架维护了一个全局配置

```
extern zconfig_t *zvar_default_config;

/* 初始化 */
zconfig_t *zdefault_config_init(void);
```

_PS: [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)底层不使用 zvar_default_config, 但一些内嵌的服务器框架使用_


## 例子: 加载配置文件

https://gitee.com/linuxmail/lib-zc/blob/master/sample/stdlib/config.c

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
        "httpd_upload_file_size_limit", 0, &var_httpd_upload_file_size_limit,
        0, 0, 0, 0, 0
    };
    zconfig_get_size_table(config, size_table);
}
```

