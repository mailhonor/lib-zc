
[C++版本](./main_argument_cpp.md)

## 通用命令行参数, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 支持 **main** 函数参数处理机制,
可以和 [全局配置(zvar_default_config)](./config.md) 无缝结合

## 参数风格, 例子

```
./cmd -name1 val1 arg1 -name2 val2 --bool1 --bool2 arg2 arg3
```

* 1个横杠 **-** 开始的是"参数名", 后面必须跟一个"参数值"
* 2个横杠 **--** 开始的是布尔型的参数, 后面没有"参数值", (逻辑上)值为真
* 其他参数为普通参数

## 全局变量和函数

### extern char *zvar_progname;

* 程序名字, 既 argv[0]

### extern int zvar_main_redundant_argc;

* 普通参数的个数

### extern char **zvar_main_redundant_argv;

* 普通参数列表的指针

### void zmain_argument_run(int argc, char **argv)

* 核心处理函数
* 可重复执行

## zmain_argument_run 执行结果说明

### 已知:

```
./cmd -name1 val1 arg1 -name2 val2 --bool1 --bool2 arg2 arg3
```

### 调用函数

```
zmain_argument_run(argc, argv);
```
### 有如下效果:

系统会初始化全局配置变量

```
char *zvar_progname = argv[0];
char **zvar_main_redundant_argv;
int zvar_main_redundant_argc;
zconfigt_t *zvar_default_config;
```
而且逻辑上:

```
zvar_default_config[name1] = val1
zvar_default_config[name2] = val2
zvar_default_config[bool1] = "yes"
zvar_default_config[bool2] = "yes"
zvar_main_redundant_argc = 3
zvar_main_redundant_argv[0] = arg1
zvar_main_redundant_argv[1] = arg2
zvar_main_redundant_argv[2] = arg3
```
### 另外

* 如果参数中出现(可以多组) **-config somepath.cf**, 那么会立即加载配置文件somepath.cf到zvar_default_config
* 遵循规则: 后加载的配置项覆盖先加载的配置项
* 遵循规则: 命令行上的配置项覆盖配置文件中的配置项
* 最后处理:

```
zvar_log_debug_enable = zconfig_get_bool(zvar_default_config, "debug", zvar_log_debug_enable);
```

## 例子 1

```
#include "zc.h"
int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv); 
    zconfig_debug_show(zvar_default_config);
    return 0;
}
```

## 例子 2

我们经常把第1个参数作为子命令, 在本框架下可以这么处理

```
zmain_argument_run(argc, argv);
char *subcmd = zvar_main_redundant_argv[0];
```

