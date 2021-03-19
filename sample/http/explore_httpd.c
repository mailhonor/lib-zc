/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-03-10
 * ================================
 */


#include "zc.h"
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>

const char *explore_data_1 =
"<!DOCTYPE html>"
"<html xmlns=\"http://www.w3.org/1999/xhtml\">"
"<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
"<title>LIBZC HTTPD EXPLORE</title>"
"<script type=\"text/javascript\">"
"var display_data="
;

const char *explore_data_2 = 
";"
"function display_paths()"
"{"
"    var po = document.getElementById(\"paths\");"
"    var ns = window.location.href.split(\"/explore/\"); ns.shift();"
"    ns = ns.join(\"/explore/\").split(\"/\");ns.unshift(\"\"); console.log(ns);"
"    var i=0;"
"    for (i=0;i<ns.length-1;i++) {"
"	var s = ns.slice(0,i+1);"
"	o=document.createElement(\"A\");"
"	o.href = \"/explore\" + s.join(\"/\") + \"/\";"
"	o.innerHTML = s.join(\"/\") + \"/\";"
"	po.append(o);"
"	o=document.createElement(\"BR\"); po.append(o);"
"    }"
""
"}"
""
"function do_display()"
"{"
"    display_paths();"
"    var table = document.getElementById(\"items\");"
"    var i=0;"
"    for (i=0;i<display_data.length;i++) {"
"        var fo=display_data[i];"
"	var tr = table.insertRow(table.rows.length);"
"	var ntd = tr.insertCell(tr.cells.length);"
"	var ttd = tr.insertCell(tr.cells.length);"
"	ttd.innerTEXT = fo.type;"
"	ttd.textContent = fo.type;"
"        var a=document.createElement(\"A\");"
"	a.innerTEXT = fo.name;"
"	a.textContent = fo.name;"
"	console.log(fo.name);"
"	if (fo.type == \"regular file\") {"
"	    a.href=fo.name;"
"	} else if (fo.type == \"directory\") {"
"	    a.href=fo.name + \"/\";"
"	} else {"
"	}"
"	ntd.append(a);"
"    }"
"}"
""
"</script>"
"</head>"
"<body onload=\"do_display();\">"
"<B id=\"paths\"></B>"
"<HR>"
"<TABLE id=\"items\" cellspacing=10>"
"<TR><TH width=\"300\">Name</TH><TH>Type</TH></TR>"
"</TABLE>"
"<HR>"
"<i>LIBZC HTTPD <A href=\"/\">homepage</A></i> "
"</body>"
"</html>"
;

typedef union {
    void *ptr;
    struct {
        int fd;
        unsigned int sock_type:8;
        unsigned int is_ssl:1;
    } sockinfo;
} long_info_t;

typedef void (*cmd_fn_t)(zhttpd_t * httpd);

static const char *www_root = 0;
static void usage()
{
    printf("USAGE: %s -listen 0:8899 [ -www_root ./ ] \n", zvar_progname);
    exit(1);
}

static void httpd_handler(zhttpd_t *httpd)
{
    char path[4096+1];
    const char *uri = zhttpd_request_get_uri(httpd);
    if (*uri == '/') {
        uri++;
    }
    snprintf(path, 4096, "%s%s", www_root, uri);

    char *p = strchr(path, '?');
    if (p) {
        *p = 0;
    }

    if (strstr(path, "..")) {
        zhttpd_response_404(httpd);
        return;
    }

    struct stat st;
    if (stat(path, &st) < 0) {
        if (errno == ENOENT) {
            zhttpd_response_404(httpd);
        } else {
            zhttpd_response_500(httpd);
        }
        return;
    }

    if (S_ISREG(st.st_mode)) {
        zhttpd_response_file(httpd, path, 0, 0);
    } else if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(path);
        if (!dir) {
            zhttpd_response_404(httpd);
            return;
        }
        zjson_t *js = zjson_create();
        struct dirent *ent;
        while((ent = readdir(dir))) {
            char *name = ent->d_name;
            if ((!strcmp(name, ".")) || (!strcmp(name, ".."))) {
                continue;
            }
            zjson_t *o = zjson_create();
            zjson_object_add(o, "name", zjson_create_string(name, -1), 0);
            const char *type = "unknown";
            unsigned char d_type = ent->d_type;
            if (d_type == DT_BLK) {
                type = "block device";
            } else if (d_type == DT_CHR) {
                type = "character device";
            } else if (d_type == DT_DIR) {
                type = "directory";
            } else if (d_type == DT_FIFO) {
                type = "fifo";
            } else if (d_type == DT_LNK) {
                type = "symbolic link";
            } else if (d_type == DT_REG) {
                type = "regular file";
            } else if (d_type == DT_SOCK) {
                type = "domain socket";
            }
            zjson_object_add(o, "type", zjson_create_string(type, -1), 0);
            zjson_array_push(js, o);
        }
        closedir(dir);
        zbuf_t *bf = zbuf_create(-1);
        zbuf_strcat(bf, explore_data_1);
        zjson_serialize(js, bf, 0);
        zbuf_strcat(bf, explore_data_2);
        zjson_free(js);
        zhttpd_response_200(httpd, zbuf_data(bf), zbuf_len(bf));
        zbuf_free(bf);
    } else {
        zhttpd_response_404(httpd);
    }
}

static void *do_httpd(void *arg)
{
    long_info_t linfo;
    linfo.ptr = arg;

    int fd = linfo.sockinfo.fd;

    zhttpd_t *httpd;
    httpd = zhttpd_open_fd(fd);

    zhttpd_set_handler(httpd, httpd_handler);
    zhttpd_run(httpd);
    zhttpd_close(httpd, 1);

    return 0;
}

static void *accept_incoming(void *arg)
{
    long_info_t linfo;
    linfo.ptr = arg;
    int sock = linfo.sockinfo.fd;
    int sock_type =linfo.sockinfo.sock_type;

    while (1) {
        ztimed_read_wait(sock, 10);
        int fd = zaccept(sock, sock_type);
        if (fd < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            if (errno == EINTR) {
                continue;
            }
            zfatal("accept: %m");
        }
        linfo.sockinfo.fd = fd;
        zcoroutine_go(do_httpd, linfo.ptr, -1);
    }
    return arg;
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    zvar_httpd_no_cache = 1;
    zmain_argument_run(argc, argv, 0);

    char *listen = zconfig_get_str(zvar_default_config, "listen", "");
    if (zempty(listen)) {
        usage();
    }
    www_root = zconfig_get_str(zvar_default_config, "www_root", "./");

    zcoroutine_base_init();

    long_info_t linfo;
    int fd, type;
    fd = zlisten(listen, &type, 5);
    if (fd < 0) {
        printf("ERR can not open %s(%m)\n", listen);
        exit(1);
    }
    linfo.sockinfo.fd = fd;
    linfo.sockinfo.sock_type = type;

    zcoroutine_go(accept_incoming, linfo.ptr, -1);

    zcoroutine_base_run(0);

    zclose(fd);

    zcoroutine_base_fini();

    zinfo("EXIT");

    return 0;
}
