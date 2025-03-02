/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-04
 * ================================
 */

#include "zcc/zcc_win64.h"
#include "zcc/zcc_errno.h"
#include <stdio.h>
#ifdef _WIN64
#include <windows.h>
#include <winbase.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wchar.h>
#include <sys/utime.h>
#include <direct.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#else // _WIN64
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <stdarg.h>
#include <dirent.h>
#endif // _WIN64

#ifndef Z_MAX_PATH
#define Z_MAX_PATH 10240
#endif // Z_MAX_PATH

#define ROBUST_DO(exp)        \
    int ret = -1;             \
    while (1)                 \
    {                         \
        ret = exp;            \
        if (ret > -1)         \
        {                     \
            return ret;       \
        }                     \
        int ec = get_errno(); \
        if (ec == ZCC_EINTR)  \
        {                     \
            continue;         \
        }                     \
        set_errno(ec);        \
        return ret;           \
    }                         \
    return ret;

#define ROBUST_DO_ONE_MORE(exp, E) \
    int ret = -1;                  \
    while (1)                      \
    {                              \
        ret = exp;                 \
        if (ret > -1)              \
        {                          \
            return ret;            \
        }                          \
        int ec = get_errno();      \
        if (ec == ZCC_EINTR)       \
        {                          \
            continue;              \
        }                          \
        if (ec == E)               \
        {                          \
            return 0;              \
        }                          \
        set_errno(ec);             \
        return ret;                \
    }                              \
    return ret;

zcc_namespace_begin;

FILE *fopen(const char *pathname, const char *mode)
{
#ifdef _WIN64
    wchar_t pathnamew[Z_MAX_PATH + 1];
    wchar_t modew[64 + 1];
    int mlen = std::strlen(mode);
    if (mlen > 10)
    {
        return 0;
    }
    if (Utf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
    {
        return 0;
    }
    if (Utf8ToWideChar(mode, mlen, modew, 64) < 1)
    {
        return 0;
    }
    return ::_wfopen(pathnamew, modew);
#else  // _Win32
    return ::fopen(pathname, mode);
#endif // _Win32
}

#ifdef _WIN64
int64_t getdelim(char **lineptr, int64_t *n, int delim, FILE *stream)
{
    if ((lineptr == 0) || (n == 0))
    {
        zcc_error("getdelim lineptr is NULL");
        return -1;
    }
    int nn = *n, count = 0;
    if (*lineptr == 0)
    {
        *n = 128;
        nn = *n;
        *lineptr = (char *)zcc::malloc(nn + 1);
    }
    while (1)
    {
        int ch = ::fgetc(stream);
        if (ch == EOF)
        {
            break;
        }
        if (count + 1 > nn)
        {
            nn *= 2;
            *lineptr = (char *)zcc::realloc(*lineptr, nn + 1);
        }
        (*lineptr)[count] = ch;
        count++;
        if (ch == delim)
        {
            break;
        }
        continue;
    }
    (*lineptr)[count] = 0;
    if (count == 0)
    {
        return -1;
    }
    return count;
}
#endif // _WIN64

std::string realpath(const char *pathname)
{
    std::string r;
    if (empty(pathname))
    {
        return r;
    }
#ifdef _WIN64
    wchar_t pathnamew[Z_MAX_PATH + 1];
    wchar_t result[Z_MAX_PATH + 1];
    if (Utf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
    {
        return r;
    }
    int ret = GetFullPathNameW(pathnamew, Z_MAX_PATH, result, 0);
    if (ret < 1)
    {
        return r;
    }
    r = WideCharToUTF8(result, ret);
    return r;
#else  // _Win32
    char buf[PATH_MAX + 1];
    char *p = ::realpath(pathname, buf);
    if (p)
    {
        r = p;
    }
    return r;
#endif // _Win32
}

#ifdef _WIN64
int stat(const char *pathname, struct _stat64i32 *statbuf)
{
    wchar_t pathnamew[Z_MAX_PATH + 1];
    if (Utf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
    {
        return 0;
    }
    return ::_wstat(pathnamew, statbuf);
}
#else  // _WIN64
int stat(const char *pathname, struct stat *statbuf)
{
    return ::stat(pathname, statbuf);
}
#endif // _WIN64

int64_t file_get_size(const char *pathname)
{
#ifdef _WIN64
    struct _stat64i32 st;
#else  // _WIN64
    struct stat st;
#endif // _WIN64
    if (zcc::stat(pathname, &st) == -1)
    {
        int ec = get_errno();
        if (ec == ZCC_ENOENT)
        {
            return 0;
        }
        return -1;
    }
    return st.st_size;
}

int file_exists(const char *pathname)
{
#ifdef _WIN64
    struct _stat64i32 st;
#else  // _WIN64
    struct stat st;
#endif // _WIN64
    if (zcc::stat(pathname, &st) == -1)
    {
        int ec = get_errno();
        if (ec == ZCC_ENOENT)
        {
            return 0;
        }
        return -1;
    }
    return 1;
}

/* ################################################################## */
/* file get/put contents */
int file_put_contents(const char *pathname, const void *data, int len)
{
    int ret;
    FILE *fp;

    if (!(fp = fopen(pathname, "wb+")))
    {
        return -1;
    }
    fwrite(data, 1, len, fp);
    ret = fflush(fp);
    fclose(fp);
    if (ret == EOF)
    {
        return -1;
    }
    return 1;
}

int64_t file_get_contents(const char *pathname, std::string &bf)
{
    FILE *fp = 0;

    bf.clear();
    if (!(fp = fopen(pathname, "rb")))
    {
        return -1;
    }
    while (1)
    {
        int ch = fgetc(fp);
        if (ch == EOF)
        {
            break;
        }
        bf.push_back(ch);
    }
    int ret = ferror(fp);
    fclose(fp);
    if (ret)
    {
        return -1;
    }
    return bf.size();
}

std::string file_get_contents(const char *pathname)
{
    std::string r;
    file_get_contents(pathname, r);
    return r;
}

int64_t file_get_contents_sample(const char *pathname, std::string &bf)
{
    int64_t ret = file_get_contents(pathname, bf);
    if (ret < 0)
    {
        zcc_error("load from %s", pathname);
        exit(1);
    }
    return ret;
}

std::string file_get_contents_sample(const char *pathname)
{
    std::string r;
    file_get_contents_sample(pathname, r);
    return r;
}

int stdin_get_contents(std::string &bf)
{
    bf.clear();
    while (1)
    {
        int ch = fgetc(stdin);
        if (ch == EOF)
        {
            break;
        }
        bf.push_back(ch);
    }
    return bf.size();
}

std::string stdin_get_contents()
{
    std::string r;
    stdin_get_contents(r);
    return r;
}

#ifdef _WIN64
static int _open_win64(const char *pathname, int flags, int mode)
{
    wchar_t pathnamew[Z_MAX_PATH + 1];
    if (Utf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
    {
        return 0;
    }
    return ::_wopen(pathnamew, flags, mode);
}
#endif // _WIN64

int open(const char *pathname, int flags, int mode)
{
#ifdef _WIN64
    ROBUST_DO(_open_win64(pathname, flags, mode));
#else  // _WIN64
    ROBUST_DO(::open(pathname, flags, mode));
#endif // _WIN64
}

int touch(const char *pathname)
{
    int fd = -1;
#ifdef _WIN64
    if ((fd = _open_win64(pathname, O_RDWR | O_CREAT, 0666)) < 0)
    {
        return -1;
    }
    ::close(fd);
    return 1;
#else  // _WIN64
    if ((fd = open(pathname, O_RDWR | O_CREAT, 0666)) < 0)
    {
        return -1;
    }
    if (futimens(fd, 0) < 0)
    {
        ::close(fd);
        return -1;
    }
    ::close(fd);
    return 1;
#endif // _WIN64
}

int mkdir(const char *pathname, int mode)
{
#ifdef _WIN64
    wchar_t pathnamew[Z_MAX_PATH + 1];
    if (Utf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
    {
        return 0;
    }
    return ::_wmkdir(pathnamew);
#else  // _WIN64
    return ::mkdir(pathname, mode);
#endif // _WIN64
}

#ifdef _WIN64
#define path_splitor '\\'
#else // _WIN64
#define path_splitor '/'
#endif // _WIN64

int mkdir(std::vector<std::string> paths, int mode)
{
    int r = -1, ret;
#ifdef _WIN64
    struct _stat64i32 st;
#else  // _WIN64
    struct stat st;
#endif // _WIN64
    std::string tmppath;
    char pathbuf[10240 + 1];
    char *path;
    unsigned char *ps, *p;
    int saved_ch;

    for (auto it = paths.begin(); it != paths.end(); it++)
    {
        const char *path = it->c_str();
        if ((!tmppath.empty()) && (tmppath.back() != path_splitor))
        {
            tmppath.push_back(path_splitor);
        }
        if (empty(path))
        {
            break;
        }
        if ((path[0] == '/') && (!tmppath.empty()))
        {
            path++;
        }
#ifdef _WIN64
        if (path[0] == '\\')
        {
            path++;
        }
#endif // _WIN64
        tmppath.append(path);
    }

#ifdef _WIN64
    for (uint64_t i = 0; i < tmppath.size(); i++)
    {
        if (tmppath[i] == '/')
        {
            tmppath[i] = '\\';
        }
    }
#endif // _WIN64
    if (tmppath.size() > 10240)
    {
        goto over;
    }
    std::memcpy(pathbuf, tmppath.c_str(), tmppath.size());
    pathbuf[tmppath.size()] = 0;
    path = pathbuf;
    ps = (unsigned char *)path;
    saved_ch = -1;
    for (; ps;)
    {
        if (saved_ch > -1)
        {
            ps[0] = saved_ch;
        }
        p = (unsigned char *)std::strchr((char *)ps, path_splitor);
        if (p)
        {
            ps = p + 1;
            saved_ch = *ps;
            *ps = 0;
        }
        else
        {
            ps = 0;
        }
        if ((ret = zcc::stat(path, &st)) < 0)
        {
            if (get_errno() == ZCC_ENOTDIR)
            {
                goto over;
            }
        }
        else
        {
#ifdef _WIN64
            if (!(st.st_mode & _S_IFDIR))
            {
                set_errno(ZCC_ENOTDIR);
                goto over;
            }
#else  // _WIN64
            if (!S_ISDIR(st.st_mode))
            {
                set_errno(ZCC_ENOTDIR);
                goto over;
            }
#endif // _WIN64
            continue;
        }

        if ((ret = mkdir(path, mode)) < 0)
        {
            {
                int ec = get_errno();
                if (ec != ZCC_EEXIST)
                {
                    goto over;
                }
                continue;
            }
        }
    }
    r = 1;
over:
    return r;
}
#undef path_splitor

int mkdir(int mode, const char *path1, ...)
{
    char *path;
    std::vector<std::string> paths;
    paths.push_back(path1);
    va_list ap;
    va_start(ap, path1);
    while ((path = (char *)va_arg(ap, char *)))
    {
        paths.push_back(path);
    }
    va_end(ap);
    return mkdir(paths, mode);
}

int rename(const char *oldpath, const char *newpath)
{
#ifdef _WIN64
    wchar_t oldpathw[Z_MAX_PATH + 1];
    wchar_t newpathw[Z_MAX_PATH + 1];
    if ((Utf8ToWideChar(oldpath, -1, oldpathw, Z_MAX_PATH) < 1) || (Utf8ToWideChar(newpath, -1, newpathw, Z_MAX_PATH) < 1))
    {
        return -1;
    }
    if (!MoveFileExW(oldpathw, newpathw, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
    {
        return -1;
    }
    return 0;
#else  // _WIN64
    ROBUST_DO(::rename(oldpath, newpath));
#endif // _WIN64
}

int unlink(const char *pathname)
{
#ifdef _WIN64
    wchar_t pathnamew[Z_MAX_PATH + 1];
    if (Utf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
    {
        return -1;
    }
    if (!DeleteFileW(pathnamew))
    {
        int ec = get_errno();
        if (ec == ZCC_ENOENT)
        {
            return 0;
        }
        return -1;
    }
    return 0;
#else  // _WIN64
    ROBUST_DO_ONE_MORE(::unlink(pathname), ZCC_ENOENT);
#endif // _WIN64
}

int link(const char *oldpath, const char *newpath)
{
#ifdef _WIN64
    wchar_t oldpathw[Z_MAX_PATH + 1];
    wchar_t newpathw[Z_MAX_PATH + 1];
    if ((Utf8ToWideChar(oldpath, -1, oldpathw, Z_MAX_PATH) < 1) || (Utf8ToWideChar(newpath, -1, newpathw, Z_MAX_PATH) < 1))
    {
        return -1;
    }
    if (!CreateHardLinkW(newpathw, oldpathw, NULL))
    {
        return -1;
    }
    return 0;
#else  // _WIN64
    ROBUST_DO(::link(oldpath, newpath));
#endif // _WIN64
}

int link_force(const char *oldpath, const char *newpath, const char *tmpdir)
{
    int ret = link(oldpath, newpath);
    if (ret == 0)
    {
        return 0;
    }
    int ec = get_errno();
    if (ec != ZCC_EEXIST)
    {
        return -1;
    }

    std::string tmppath;
    tmppath.append(tmpdir).append("/");
    tmppath.append(build_unique_id());

    ret = link(oldpath, tmppath.c_str());
    if (ret < 0)
    {
        return -1;
    }
    ret = rename(tmppath.c_str(), newpath);
    unlink(tmppath.c_str());
    if (ret < 0)
    {
        return -1;
    }
    return 0;
}

int symlink(const char *oldpath, const char *newpath)
{
#ifdef _WIN64
    DWORD attr;
    BOOLEAN res;
    wchar_t oldpathw[Z_MAX_PATH + 1];
    wchar_t newpathw[Z_MAX_PATH + 1];
    if ((Utf8ToWideChar(oldpath, -1, oldpathw, Z_MAX_PATH) < 1) || (Utf8ToWideChar(newpath, -1, newpathw, Z_MAX_PATH) < 1))
    {
        return -1;
    }
    if ((attr = GetFileAttributesW(oldpathw)) == INVALID_FILE_ATTRIBUTES)
    {
        return -1;
    }
    res = CreateSymbolicLinkW(oldpathw, oldpathw, (attr & FILE_ATTRIBUTE_DIRECTORY ? 1 : 0));
    if (!res)
    {
        return -1;
    }
    return 0;
#else  // _WIN64
    ROBUST_DO(::symlink(oldpath, newpath));
#endif // _WIN64
}

int symlink_force(const char *oldpath, const char *newpath, const char *tmpdir)
{
    int ret = symlink(oldpath, newpath);
    if (ret == 0)
    {
        return 0;
    }
    int ec = get_errno();
    if (ec != ZCC_EEXIST)
    {
        return -1;
    }

    std::string tmppath;
    tmppath.append(tmpdir).append("/");
    tmppath.append(build_unique_id());
    ret = symlink(oldpath, tmppath.c_str());
    if (ret < 0)
    {
        return -1;
    }
    ret = rename(tmppath.c_str(), newpath);
    if (ret < 0)
    {
        unlink(tmppath.c_str());
        return -1;
    }
    return 0;
}

static int _rmdir_true_do(const char *pathname)
{
#ifdef _WIN64
    wchar_t pathnamew[Z_MAX_PATH + 1];
    if (Utf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
    {
        return -1;
    }
    if (_wrmdir(pathnamew) < 0)
    {
        int ec = get_errno();
        if (ec == ZCC_ENOENT)
        {
            return 0;
        }
        set_errno(ec);
        return -1;
    }
    return 0;
#else  // _WIN64
    ROBUST_DO_ONE_MORE(::rmdir(pathname), ENOENT);
#endif // _WIN64
}

int rmdir(const char *pathname, bool recurse_mode)
{
    std::string path;
    std::string filename;
    std::vector<dir_item_info> items;
    int ret = -1;

    if (!recurse_mode)
    {
        return _rmdir_true_do(pathname);
    }

    if ((ret = scandir(pathname, items)) < 0)
    {
        goto over;
    }
    for (auto it = items.begin(); it != items.end(); it++)
    {
        path.clear();
        path.append(filename).append("/").append(it->filename);
        if (it->dir)
        {
            if (_rmdir_true_do(path.c_str()) < 1)
            {
                goto over;
            }
        }
        else
        {
            if (unlink(path.c_str()) < 1)
            {
                goto over;
            }
        }
    }
    ret = 1;

over:
    return ret;
}

int scandir(const char *dirname, std::vector<dir_item_info> &filenames)
{
#ifdef _WIN64
    int ret = -1;
    WIN32_FIND_DATAW FindFileData;
    HANDLE hFind;

    std::string tmpdirname = dirname;
    if (!tmpdirname.empty())
    {
        if ((tmpdirname.back() == '\\') || (tmpdirname.back() == '/'))
        {
            tmpdirname.pop_back();
        }
        tmpdirname.append("\\*.*");
    }
    dirname = tmpdirname.c_str();

    std::wstring pw = Utf8ToWideChar(dirname);
    hFind = FindFirstFileW(pw.c_str(), &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return -1;
    }
    while (1)
    {
        std::string filename = WideCharToUTF8((wchar_t *)FindFileData.cFileName);
        const char *fn = filename.c_str();
        if (!((fn[0] == '.') && ((fn[1] == '\0') || ((fn[1] == '.') && (fn[2] == '\0')))))
        {
            dir_item_info item;
            item.filename = filename;
            // FIXME
            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                item.dir = true;
            }
            filenames.push_back(item);
        }
        if (FindNextFileW(hFind, &FindFileData) == 0)
        {
            if (GetLastError() == ERROR_NO_MORE_FILES)
            {
                break;
            }
            goto over;
        }
    }
    ret = 1;

over:
    FindClose(hFind);
    return ret;
#else  // _WIN64
    DIR *dir;
    struct dirent *ent_list;

    if (!(dir = opendir(dirname)))
    {
        if (get_errno() == ZCC_ENOENT)
        {
            return 0;
        }
        zcc_error("访问文件夹失败:%s(%m)", dirname);
        return -1;
    }

    // modern linux, readdir is thread-safe
    while ((ent_list = readdir(dir)))
    {
        const char *fn = ent_list->d_name;
        if ((fn[0] == '.') && ((fn[1] == '\0') || ((fn[1] == '.') && (fn[2] == '\0'))))
        {
            continue;
        }
        dir_item_info item;
        item.filename = fn;
        if (ent_list->d_type == DT_BLK)
        {
            item.dev = true;
        }
        else if (ent_list->d_type == DT_CHR)
        {
            item.dev = true;
        }
        else if (ent_list->d_type == DT_DIR)
        {
            item.dir = true;
        }
        else if (ent_list->d_type == DT_FIFO)
        {
            item.fifo = true;
        }
        else if (ent_list->d_type == DT_LNK)
        {
            item.link = true;
        }
        else if (ent_list->d_type == DT_REG)
        {
            item.regular = true;
        }
        else if (ent_list->d_type == DT_SOCK)
        {
            item.socket = true;
        }
        filenames.push_back(item);
    }
    closedir(dir);
    return 1;
#endif // _WIN64
}

std::vector<dir_item_info> scandir(const char *dirname)
{
    std::vector<dir_item_info> filenames;
    scandir(dirname, filenames);
    return filenames;
}

std::string format_filename(const char *filename)
{
    std::string path;
    const char *ignore = "/\\|";
#ifdef _WIN64
    ignore = "\":<>?/\\|*";
#endif
    for (const unsigned char *p = (const unsigned char *)(void *)filename; *p; p++)
    {
        int ch = *p;
        if (ch < 127)
        {
            if (!::std::isprint(ch))
            {
                ch = '_';
            }
            else if (std::strchr(ignore, ch))
            {
                ch = '_';
            }
        }
        path.push_back(ch);
    }
    return path;
}

std::vector<std::string> find_file_sample(std::vector<const char *> dir_or_file, const char *pathname_match)
{
    return find_file_sample(dir_or_file.data(), (int)dir_or_file.size(), pathname_match);
}

std::vector<std::string> find_file_sample(const char **dir_or_file, int item_count, const char *pathname_match)
{
    std::vector<std::string> r;
#ifdef _WIN64
    return r;
#else  // _WIN64
    std::map<std::string, bool> rs;
    char buf[4096 + 1];
    for (int i = 0; i < item_count; i++)
    {
        const char *pathname = dir_or_file[i];
        struct stat st;
        if (::stat(pathname, &st) == -1)
        {
            if (errno == ENOENT)
            {
                continue;
            }
            zcc_error_and_exit("open %s(%m)", pathname);
        }
        if (S_ISREG(st.st_mode))
        {
            if (rs.find(pathname) == rs.end())
            {
                r.push_back(pathname);
                rs[pathname] = true;
            }
            continue;
        }
        else if (!S_ISDIR(st.st_mode))
        {
            zcc_debug("WARNING file must be regular file or directory %s", pathname);
            continue;
        }
        if (zcc::empty(pathname_match))
        {
            std::sprintf(buf, "find \"%s\" -type f", pathname);
        }
        else
        {
            std::sprintf(buf, "find \"%s\" -type f -name \"%s\"", pathname, pathname_match);
        }
        FILE *fp = popen(buf, "r");
        if (!fp)
        {
            zcc_debug("ERROR popen: find \"%s\" -type f", pathname);
        }
        while (fgets(buf, 4096, fp))
        {
            char *p = strchr(buf, '\n');
            if (p)
            {
                *p = 0;
            }
            p = strchr(buf, '\r');
            if (p)
            {
                *p = 0;
            }
            if (rs.find(buf) == rs.end())
            {
                r.push_back(buf);
                rs[buf] = true;
            }
        }
        fclose(fp);
    }
    return r;
#endif // _WIN64
}

bool create_shortcut_link(const char *from, const char *to)
{
#ifdef _WIN64
    if (FAILED(CoInitialize(NULL)))
    {
        return false;
    }

    HRESULT hres;
    IShellLinkW *psl;

    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID *)&psl);
    if (!SUCCEEDED(hres))
    {
        return false;
    }
    std::wstring from_path = Utf8ToWideChar(std::string(from));
    std::wstring to_path = Utf8ToWideChar(std::string(to));

    psl->SetPath((LPCWSTR)from_path.c_str());
    psl->SetWorkingDirectory((LPCWSTR)from_path.c_str());

    IPersistFile *ppf;
    hres = psl->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf);

    if (SUCCEEDED(hres))
    {
        hres = ppf->Save((LPCOLESTR)to_path.c_str(), TRUE);
        ppf->Release();
    }
    psl->Release();
    return true;
#else  // _WIN64
    return false;
#endif // _WIN32
}

zcc_namespace_end;
