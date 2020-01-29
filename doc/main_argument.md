# 命令行参数

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

内嵌main函数参数处理机制, 可以和全局[通用配置](./config.md)无缝结合, 推荐使用

## 全局变量和函数

```
extern char *zvar_progname;
extern char **zvar_main_redundant_argv;
extern int zvar_main_redundant_argc;
void zmain_argument_run(int argc, char **argv, unsigned int (*self_argument_fn)(int argc, char **argv, int offset));
```

## 参数风格

./cmd -name1 val1 arg1 -name2 val2 --bool1 --bool2 ... arg2 arg3

- 1个横杠 - 开始的是"参数名", 后面必须跟一个"参数值"
- 2个横杠 --开始的是布尔型的参数, 后面没有"参数值". 
 (逻辑上)值为真
- 其他参数为普通参数

对上述例子执行函数 

```
zmain_argument_run(argc, argv, 0) ;
```

有如下效果:


1, 系统会初始化全局配置变量 ```zconfigt_t *zvar_default_config;```


2,  而且逻辑上

```
   zvar_default_config[name1] = val1
   zvar_default_config[name2] = val2
   zvar_default_config[bool1] = "yes"
   zvar_default_config[bool2] = "yes"
   zvar_main_redundant_argc = 3
   zvar_main_redundant_argv[0] = arg1
   zvar_main_redundant_argv[1] = arg2
   zvar_main_redundant_argv[3] = arg3
```

3,  main_argument_run 在 最后处理:

```
if (zconfig_get_bool(zvar_default_config, "debug", 0) == 1) {
	zvar_log_debug_enable = 1
}
```

4, 如果参数中出现(可以多组)

```
-config somepath.cf
```
那么 会立即加载配置文件somepath.cf到zvar_default_config

5, 遵循规则

- 后加载的配置项覆盖先加载的配置项
- 命令行上的配置项覆盖配置文件中的配置项

6, 同时执行

```
zvar_progname = argv[0]; 
```

7, 同时初始化了

```
char **zvar_main_redundant_argv;
int zvar_main_redundant_argc;
```
## 例子

```
#include "zc.h"
int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv, 0); 
    zconfig_debug_show(zvar_default_config);
    return 0;
}
```

## 扩展: zmain_argument_run第三个参数

*PS: 一般来说, 上面的用法足够满足大部分需求了,如果需要处理更复杂的参数请继续*

第三个参数是一个回调函数, 用于处理自定义参数, 可以是 0

```
unsigned int somefunction(int argc, char **argv, int offset);
```

原理(伪码)是这样的

```
void zmain_argument_run(int argc, char **argv, unsigned int (*self_argument_fn)(int argc, char **argv, int offset))
{
	for  (int i = 1; i < argc; i++) {
		if (selfself_argument_fn) {
			int jump = selfself_argument_fn(argc, argc, i);
			if (jump > 0) {
				i += jump - 1;
				continue;
			}
		}
		...;
	}
}
```

下面举个例字:

*我们经常把第1个参数作为子命令*

在本框架下可以这么处理

```
char * subcmd= 0;
static unsigned int deal_argument(int argc, char **argv, int offset)
{
	if(offset != 1) {
		return 0;
	}
	subcmd = argv[1];
	if (!strcmp(subcmd, "cmd1")) {
		return 1;
	} else if(!strcmp(subcmd, "cmd2"))) {
		return 1;
	}
	subcmd = 0;
	return 0;
}
zmain_argument_run(argc, argv, deal_argument);
```
