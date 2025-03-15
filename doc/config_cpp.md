<A name="readme_md" id="readme_md"></A>

[C版本](./config.md)

## key = value 型通用配置, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 支持通用配置,

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

```c++
// 线程, 读写不安全
// 线程, 只有读是安全的
namespace zcc {
class config : public std::map<std::string, std::string>
{
public:
    config();
    ~config();
    config &reset();
    virtual inline void afterUpdate() {};
    config &update(const char *key, const char *val, int vlen = -1);
    config &update(const char *key, const std::string &val);
    config &update(const std::string &key, const std::string &val);
    config &remove(const char *key);
    config &remove(const std::string &key);
    // 从文件加载配置, 且覆盖
    bool load_from_file(const char *pathname);
    bool load_from_file(const std::string &pathname)
    {
        return load_from_file(pathname.c_str());
    }
    // 从另一个配置复制
    config &load_another(config &another);
    config &debug_show();
    // 获取值
    std::string *get_value(const char *key);
    std::string *get_value(const std::string &key);
    const char *get_cstring(const char *key, const char *def_val = "");
    const char *get_cstring(const std::string &key, const char *def_val = "");
    std::string get_string(const char *key, const char *def_val = "");
    std::string get_string(const std::string &key, const char *def_val = "");
    const std::string &get_string(const std::string &key, const std::string &def_val);
    // y/Y/t/T/1 => true, n/N/f/F/0 => false
    bool get_bool(const char *key, bool def_val = false);
    bool get_bool(const std::string &key, bool def_val = false);
    // atoi
    int get_int(const char *key, int def_val = 0);
    int get_int(const std::string &key, int def_val = 0);
    //atol
    int64_t get_long(const char *key, int64_t def_val = 0);
    int64_t get_long(const std::string &key, int64_t def_val = 0);
    // 1s, 1m, 1h, 1d, 1w
    int64_t get_second(const char *key, int64_t def_val = 0);
    int64_t get_second(const std::string &key, int64_t def_val = 0);
    // 1b, 1k, 1m, 1g
    int64_t get_size(const char *key, int64_t def_val = 0);
    int64_t get_size(const std::string &key, int64_t def_val = 0);

private:
    void *unused_;
};
// 默认的config, 只要使用 zcc::main_argument, 就会激活
extern config var_main_config;
}
```

