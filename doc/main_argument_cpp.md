
[C版本](./main_argument.md)

## 通用命令行参数, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 支持 **main** 函数参数处理机制,
可以和 [全局配置(zcc::var_main_config)](./config_cpp.md) 无缝结合

命名空间 zcc::main_argument

## 参数风格, 例子

```
./cmd -name1 val1 arg1 -name2 val2 --bool1 --bool2 arg2 arg3
```

* 1个横杠 **-** 开始的是"参数名", 后面必须跟一个"参数值"
* 2个横杠 **--** 开始的是布尔型的参数, 后面没有"参数值", (逻辑上)值为真
* 其他参数为普通参数

## 全局变量和函数

```c++
struct option
{
    const char *key;
    const char *val;
};

// main 函数入口的 argc
extern int var_argc;
// main 函数入口的 argv
extern char **var_argv;
// 命令行参数的 key/value 对
extern std::vector<option> var_options;
// 命令行的参数
extern std::vector<const char *> var_parameters;
// var_parameters.size()
extern int var_parameter_argc;
// (char **)(var_parameters.c_str())
extern char **var_parameter_argv;
// 解析命令行
void run(int argc, char **argv, bool cmd_mode = true);
```

## run 执行结果说明

### 已知:

```c++
./cmd -name1 val1 arg1 -name2 val2 --bool1 --bool2 arg2 arg3
```

### 调用函数

```c++
zcc::main_argument::run(argc, argv);
```
### 有如下效果:

系统会初始化全局配置变量

```
var_argc = argc;
var_argv = argv;
progname = argv[0];
```
而且逻辑上:

```c++
// var_main_config
zcc::var_main_config[name1] = val1
zcc::var_main_config[name2] = val2
zcc::var_main_config[bool1] = "yes"
zcc::var_main_config[bool2] = "yes"

// var_options
var_options = {{name1, val1}, {name2, val2}, {boo1, yes}, {bool2, yes}}

// var_parameters
var_parameters = {arg1, arg2, arg3}
```
### 另外

* 如果参数中出现(可以多组) **-config somepath.cf**, 那么会立即加载配置文件somepath.cf到 zcc::var_main_config
* 遵循规则: 后加载的配置项覆盖先加载的配置项
* 遵循规则: 命令行上的配置项覆盖配置文件中的配置项
* 最后处理:

## 例子 1

```
#include "zcc/zcc_stdlib.h"
int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv); 
    zcc::var_default_config.show_debug();
    return 0;
}
```

## 例子 2

我们经常把第1个参数作为子命令或模块, 在本框架下可以这么处理

```c++
char *subcmd = zcc::main_redundant::var_parameter_argv[0];
```

