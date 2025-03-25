
[C版本](./file.md)

## 文件操作, [LIB-ZC](./README.md)

命名空间: zcc

如果出现和标准C库同名函数, 请使用zcc版本, 因为zcc版本考虑了跨平台


```c++
// 文件映射
class mmap_reader
{
public:
    mmap_reader();
    ~mmap_reader();
    // 打开一个文件
    int open(const char *pathname); 
    inline int open(const std::string &pathname)
    {
        return open(pathname.c_str());
    }
    int close();

protected:
#ifdef _WIN64
    void *fd_{ZCC_VOID_PTR_ONE};
    void *fm_{nullptr};
#else  // _WIN64
    int fd_{-1};
#endif // _WIN64

public:
    int64_t size_{-1};          // 映射后, 长度
    const char *data_{nullptr}; // 映射后, 指针
};

// 同标准C库的fopen, 只不过, 本实现考虑了pathname的字符集转换
FILE *fopen(const char *pathname, const char *mode);
inline FILE *fopen(const std::string &pathname, const char *mode)
{
    return fopen(pathname.c_str(), mode);
}

#ifdef _WIN64
// windows 的 C库没有 getdelim
int64_t getdelim(char **lineptr, int64_t *n, int delim, FILE *stream);
inline int64_t getline(char **lineptr, int64_t *n, FILE *stream);
#else  // _WIN64
inline int64_t getdelim(char **lineptr, int64_t *n, int delim, FILE *stream)
{
    return ::getdelim(lineptr, (size_t *)n, delim, stream);
}
inline int64_t getline(char **lineptr, int64_t *n, FILE *stream)
{
    return ::getline(lineptr, (size_t *)n, stream);
}
#endif // _WIN64

// 真实路径
std::string realpath(const char *pathname);
inline std::string realpath(const std::string &pathname)
{
    return realpath(pathname.c_str());
}
#ifdef _WIN64
#define zcc_stat struct _stat64i32
int stat(const char *pathname, struct _stat64i32 *statbuf);
#else // _WIN64
#define zcc_stat struct stat
int stat(const char *pathname, struct stat *statbuf);
#endif // _WIN64

// 文件大小
int64_t file_get_size(const char *pathname);
inline int64_t file_get_size(const std::string &pathname)
{
    return file_get_size(pathname.c_str());
}
// 文件是否存在
int file_exists(const char *pathname);
inline int file_exists(const std::string &pathname)
{
    return file_exists(pathname.c_str());
}
// 覆盖写文件
int file_put_contents(const char *pathname, const void *data, int len);
inline int file_put_contents(const std::string &pathname, const void *data, int len)
{
    return file_put_contents(pathname.c_str(), data, len);
}
inline int file_put_contents(const char *pathname, const std::string &data)
{
    return file_put_contents(pathname, data.c_str(), data.size());
}
inline int file_put_contents(const std::string &pathname, const std::string &data)
{
    return file_put_contents(pathname.c_str(), data.c_str(), data.size());
}
// 读取文件内容
std::string file_get_contents(const char *pathname);
inline std::string file_get_contents(const std::string &pathname)
{
    return file_get_contents(pathname.c_str());
}
int64_t file_get_contents(const char *pathname, std::string &bf);
inline int64_t file_get_contents(const std::string &pathname, std::string &bf)
{
    return file_get_contents(pathname.c_str(), bf);
}
// 测试脚本用, 如果文件不存在则进程退出
int64_t file_get_contents_sample(const char *pathname, std::string &bf);
std::string file_get_contents_sample(const char *pathname);
// 从标准输入读取
int stdin_get_contents(std::string &bf);
std::string stdin_get_contents();

// ::open
int open(const char *pathname, int flags, int mode);
inline int open(const std::string &pathname, int flags, int mode)
{
    return open(pathname.c_str(), flags, mode);
}
// 类似命令 touch
int touch(const char *pathname);
inline int touch(const std::string &pathname)
{
    return touch(pathname.c_str());
}
//
int mkdir(std::vector<std::string> paths, int mode = 0666);
int mkdir(int mode, const char *path1, ...);
int mkdir(const char *pathname, int mode = 0666);
//
int rename(const char *oldpath, const char *newpath);
//
int unlink(const char *pathname);
inline int unlink(const std::string &pathname);
int link(const char *oldpath, const char *newpath);
int link_force(const char *oldpath, const char *newpath, const char *tmpdir);
//
int symlink(const char *oldpath, const char *newpath);
int symlink_force(const char *oldpath, const char *newpath, const char *tmpdir);
// 创建快捷方式(windows)
bool create_shortcut_link(const char *from, const char *to);
inline bool create_shortcut_link(const std::string &from, const std::string &to);
int rmdir(const char *pathname, bool recurse_mode = false);
inline int rmdir(const std::string &pathname, bool recurse_mode = false);
struct dir_item_info
{
    std::string filename;
    bool dir{false};
    bool regular{false};
    bool fifo{false};
    bool link{false};
    bool socket{false};
    bool dev{false};
};

int scandir(const char *dirname, std::vector<dir_item_info> &filenames);
inline int scandir(const std::string &dirname, std::vector<dir_item_info> &filenames);
std::vector<dir_item_info> scandir(const char *dirname);
inline std::vector<dir_item_info> scandir(const std::string &dirname);

// filename作为一个文件名, 去除不合法的字符
std::string format_filename(const char *filename);
inline std::string format_filename(const std::string &filename);

// 测试脚本用, Linux平台 find 命令封装
std::vector<std::string> find_file_sample(std::vector<const char *> dir_or_file, const char *pathname_match = nullptr);
std::vector<std::string> find_file_sample(const char **dir_or_file, int item_count, const char *pathname_match = nullptr);
```