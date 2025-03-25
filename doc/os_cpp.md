
[C版本](./file.md)

## 文件操作, [LIB-ZC](./README.md)

命名空间: zcc

```c++
// 进程 ID
int get_process_id();
inline int getpid() { return get_process_id(); }

// 父进程ID
int get_parent_process_id();
inline int getppid() { return get_parent_process_id(); }

// 线程ID
int get_thread_id();
inline int gettid() { return get_thread_id(); }

// 程序真实的绝对路径名称
std::string get_cmd_pathname();
// 程序真实的名字
std::string get_cmd_name();

#ifdef __linux__
bool quick_setrlimit(int cmd, unsigned long cur_val);
bool set_core_file_size(int megabyte);
bool set_max_mem(int megabyte);
bool set_cgroup_name(const char *name);
int64_t get_MemAvailable();
int get_cpu_core_count();
int chroot_user(const char *root_dir, const char *user_name);
#endif // __linux__
```