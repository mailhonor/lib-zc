/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn
 * 2019-11-11
 * ================================
 */

#ifdef TEST_USE_COROUTINE
#include "zc_coroutine.h"
#endif // TEST_USE_COROUTINE

#include "zcc/zcc_http.h"
#include "zcc/zcc_openssl.h"
#include "zcc/zcc_errno.h"
#include "zcc/zcc_json.h"
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

class my_httpd : public zcc::httpd
{
public:
    my_httpd(zcc::stream &stream);
    ~my_httpd();
    zcc::json *get_all_info_json();
    void do_main_page();
    void do_explore_page();
    void do_upload_page();
    void do_upload_action();
    void do_websocketd_page();
    void do_websocketd_action();
};

static const char *listen = nullptr;
static bool is_ssl = false;
static SSL_CTX *openssl_server_ctx = nullptr;

static std::string main_page_data =
    "<!DOCTYPE html>"
    "<html xmlns=\"http://www.w3.org/1999/xhtml\">"
    "<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
    "<title>LIBZC HTTPD TEST MAIN PAGE</title>"
    "</head>"
    "<body>"
    "<ul>"
    "	<li><A href=\"/explore/\">explore file system</A></li>"
    "	<li><A href=\"/upload_page/\">upload page</A></li>"
    "	<li><A href=\"/websocketd/\">websocketd</A></li>"
    "</ul>"
    "</body>"
    "</html>";

static std::string explore_page_data__1 =
    "<!DOCTYPE html>"
    "<html xmlns=\"http://www.w3.org/1999/xhtml\">"
    "<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
    "<title>LIBZC HTTPD EXPLORE</title>"
    "<script type=\"text/javascript\">"
    "var display_data=";

static std::string explore_page_data__2 =
    "    ;"
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
    "   var a=document.createElement(\"A\");"
    "	a.innerTEXT = fo.name;"
    "	a.textContent = fo.name;"
    "	console.log(fo.name);"
    "	if (fo.type == \"regular\") {"
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
    "</html>";

static std::string upload_page_data =
    "<!DOCTYPE html>"
    "<html xmlns=\"http://www.w3.org/1999/xhtml\">"
    "<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
    "<title>LIBZC HTTPD UPLOAD PAGE</title>"
    "</head>"
    "<body>"
    "<form name=\"form\" id=\"from\" method=\"post\" enctype=\"multipart/form-data\" action=\"/upload_do\"> "
    "NO.1 <input type=\"file\" name=\"name1\" value=\"\" multiple />"
    "<br>"
    "NO.2 <input type=\"file\" name=\"name2\" value=\"\" multiple />"
    "<br>"
    "DESC <input type=\"text\" name=\"description\" value=\"\" />"
    "<br>"
    "<input type=\"submit\" name=\"submit\" value=\"upload\" />"
    "</form>"
    "</body>"
    "</html>";

static std::string websocketed_page_data =
    "<!DOCTYPE html>"
    "<html xmlns=\"http://www.w3.org/1999/xhtml\">"
    "<head>"
    "    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
    "    <title>LIBZC HTTPD WEBSOCKET</title>"
    "    <script type=\"text/javascript\">"
    "        var ws = 0;"
    "        function show_msg(msg) {"
    "            var ta = document.getElementById(\"board\");"
    "            if (ta.value.length > 1024 * 1024) {"
    "                ta.value.length = \"\";"
    "            }"
    "            ta.value += \"\\n\" + msg;"
    "            ta.scrollTop = ta.scrollHeight;"
    "        }"
    "        function do_connect() {"
    "            if (ws) {"
    "                return;"
    "            }"
    "            var url = ((window.location.protocol == \"https\" ? \"wss\" : \"ws\")) + \"://\" + window.location.href.split(\"://\")[1];"
    "                console.log(url);"
    "            try {"
    "                ws = new WebSocket(url);"
    "            } catch {"
    "                show_msg(\"连接失败: \" + url);"
    "                ws = 0;"
    "                return;"
    "            }"
    "            ws.onopen = function () { show_msg(\"连接成功: \" + url); };"
    "            ws.onmessage = function (msg) { show_msg(\"收取消息: \" + msg.data); };"
    "            ws.onclose = function (msg) { show_msg(\"连接断开\"); ws = 0; };"
    "        }"
    "        function send_wrap(msg) {"
    "            if (!ws) {"
    "                return;"
    "            }"
    "            if (ws.readyState == 1) {"
    "                ws.send(msg);"
    "                return;"
    "            }"
    "            setTimeout(function () {"
    "                send_wrap(msg);"
    "            }, 1);"
    "        }"
    "        function do_send() {"
    "            if (!ws) {"
    "                do_connect();"
    "            }"
    "            if (!ws) {"
    "                show_msg(\"连接失败\");"
    "                return;"
    "            }"
    "            send_wrap(document.getElementById(\"input\").value);"
    "        }"
    "    </script>"
    "</head>"
    "<body style=\"padding:20px;\">"
    "    <textarea id=\"board\" rows=\"10\" style=\"width:500px;padding: 20px 0;\"></textarea>"
    "    <BR /> <BR />"
    "    <input id=\"input\" name=\"input\" style=\"width:500px\" />"
    "    <BR />"
    "    <input type=\"button\" value=\"发送\" onclick=\"do_send()\" />"
    "</body>"
    "</html>";

static void usage()
{
    zcc_info("USAGE: %s -listen 0:8899 [ --ssl [ -cert ./ssl.cert, -key ./ssl.key ] ]", zcc::progname);
    zcc_exit(1);
}

static void init()
{
    listen = zcc::var_main_config.get_cstring("listen");
    if (zcc::empty(listen))
    {
        usage();
    }

    is_ssl = zcc::var_main_config.get_bool("ssl");
    if (!is_ssl)
    {
        return;
    }

    zcc::openssl::env_init();
    const char *cert = zcc::var_main_config.get_cstring("cert", "ssl.cert");
    const char *key = zcc::var_main_config.get_cstring("key", "ssl.key");
    openssl_server_ctx = zcc::openssl::SSL_CTX_create_server(cert, key);
    if (!openssl_server_ctx)
    {
        char error_buf[1024];
        zcc::openssl::get_error(0, error_buf, 1024);
        zcc_info("ERROR can load cert/key:%s", error_buf);
        usage();
    }
}

static void fini()
{
}

static zcc::json *___dict_to_json(const zcc::dict &dict)
{
    auto js = new zcc::json(zcc::json_type_object);
    for (auto it = dict.begin(); it != dict.end(); it++)
    {
        js->object_update(it->first, it->second);
    }
    return js;
}

my_httpd::my_httpd(zcc::stream &stream) : httpd(stream)
{
}

my_httpd::~my_httpd()
{
}

void my_httpd::do_main_page()
{
    response_200(main_page_data);
}

zcc::json *my_httpd::get_all_info_json()
{
    auto js = new zcc::json(zcc::json_type_object);
    js->object_update("headers", ___dict_to_json(request_get_headers()));
    js->object_update("cookies", ___dict_to_json(request_get_cookies()));
    js->object_update("query_vars", ___dict_to_json(request_get_query_vars()));
    js->object_update("post_vars", ___dict_to_json(request_get_post_vars()));
    return js;
}

void my_httpd::do_explore_page()
{
    int err;
    zcc_stat st;
    const char *path = request_get_url().path_.c_str();
    if (strncmp(path, "/explore/", 9))
    {
        response_404();
        return;
    }
    path += 8;

    if (zcc::stat(path, &st) < 0)
    {
        err = zcc::get_errno();
        if (err == ZCC_ENOENT)
        {
            response_404();
            return;
        }
        else
        {
            response_500();
            return;
        }
    }
    if (std::strstr(path, "../"))
    {
        response_404();
        return;
    }
    auto ftype = (st.st_mode & S_IFMT);

    if (ftype == S_IFREG)
    {
        response_file(path);
        return;
    }
    else if (ftype != S_IFDIR)
    {
        response_404();
        return;
    }
    zcc::json js(zcc::json_type_array);
    auto items = zcc::scandir(path);
    for (auto it = items.begin(); it != items.end(); it++)
    {
        std::string type = "unknown";
        if (it->dir)
        {
            type = "directory";
        }
        else if (it->regular)
        {
            type = "regular";
        }
        else if (it->dev)
        {
            type = "device";
        }
        else if (it->fifo)
        {
            type = "fifo";
        }
        else if (it->socket)
        {
            type = "socket";
        }
        else if (it->link)
        {
            type = "link";
        }
        auto o = new zcc::json();
        js.array_push(o);
        (*o)["name"] = it->filename;
        (*o)["type"] = type;
    }
    std::string jsstr;
    js.serialize(jsstr);
    std::string data = explore_page_data__1;
    data.append(jsstr).append(explore_page_data__2);
    response_200(data);
}

void my_httpd::do_upload_page()
{
    response_200(upload_page_data);
}

void my_httpd::do_upload_action()
{
    zcc::json js(zcc::json_type_object);
    js.object_update("info", get_all_info_json());
    auto files = request_get_uploaded_files();
    int file_id = 1;
    auto file_list_js = js.object_update("files", new zcc::json(zcc::json_type_array), true);
    for (auto it = files.begin(); it != files.end(); it++)
    {
        auto tmpjs = new zcc::json();
        file_list_js->array_push(tmpjs);
        tmpjs->object_update("name", it->get_name());
        tmpjs->object_update("pathname", it->get_pathname());
        tmpjs->object_update("size", it->get_size());
        char saved_pathname[1024];
        std::sprintf(saved_pathname, "uploaded_files/%d.dat", file_id++);
        it->save_to(saved_pathname);
        tmpjs->object_update("saved_pathname", saved_pathname);
    }

    std::string con = "<!DOCTYPE html><html xmlns=\"http://www.w3.org/1999/xhtml\"><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><title>LIBZC HTTPD UPLOAD RESULT</title><script charset='UTF-8'>var data=";
    js.serialize(con);
    con.append(";console.log(data);</script></head><body>SEE console.log</body></html>");
    response_200(con);
}

void my_httpd::do_websocketd_page()
{
    response_200(websocketed_page_data);
}

static void do_websocketd_serve(zcc::stream &fp)
{
    zcc::websocketd wsd(fp);
    std::string rdata, tmp_rdata;
    while (1)
    {
        if (!wsd.read_frame_header())
        {
            break;
        }
        bool fin = wsd.get_header_fin();
        int payload_len = wsd.get_payload_len();
        if (rdata.empty())
        {
            if (wsd.read_frame_data(rdata, payload_len) < payload_len)
            {
                break;
            }
        }
        else
        {
            if (wsd.read_frame_data(tmp_rdata, payload_len) < payload_len)
            {
                break;
            }
            rdata.append(tmp_rdata);
        }
        if (!fin)
        {
            continue;
        }
        int opcode = wsd.get_header_opcode();
        bool r = false;
        if (opcode == zcc::websocketd::opcode_ping)
        {
            r = wsd.send_pong(rdata);
        }
        else if (opcode == zcc::websocketd::opcode_pong)
        {
            r = true;
        }
        else if (opcode == zcc::websocketd::opcode_text)
        {
            r = wsd.send_text("your: (see next)");
            if (r)
            {
                r = wsd.send_text(rdata);
            }
        }
        else if (opcode == zcc::websocketd::opcode_text)
        {
            r = wsd.send_text("your: (see next)");
            if (r)
            {
                r = wsd.send_text(rdata);
            }
        }
        rdata.clear();
    }
}

static void *do_websocketd_coroutine(void *arg)
{
    do_websocketd_serve(*(zcc::stream *)arg);
    return arg;
}

void my_httpd::do_websocketd_action()
{
    if (!websocket_shakehand())
    {
        response_500();
        return;
    }
    auto fp = detach_stream();
// 如果是协程/线程等并发环境, 可以 释放 my_httpd 了
#ifdef TEST_USE_COROUTINE
    zcoroutine_go(do_websocketd_coroutine, fp, -1);
#else  // TEST_USE_COROUTINE
    do_websocketd_serve(*fp);
#endif // TEST_USE_COROUTINE
}

static void do_httpd_serve_once(my_httpd &httpd)
{
    if (!httpd.request_read_all())
    {
        return;
    }
    const char *path = httpd.request_get_url().path_.c_str();
    if (path[0] == '/')
    {
        path++;
    }
    if (zcc::empty(path))
    {
        httpd.do_main_page();
    }
    else if (!strncmp(path, "explore/", 8))
    {
        httpd.do_explore_page();
    }
    else if (!strncmp(path, "upload_page/", 12))
    {
        httpd.do_upload_page();
    }
    else if (!strncmp(path, "upload", 6))
    {
        httpd.do_upload_action();
    }
    else if (!strncmp(path, "websocketd/", 11))
    {
        if (!httpd.is_websocket_upgrade())
        {
            httpd.do_websocketd_page();
        }
        else
        {
            httpd.do_websocketd_action();
        }
    }
    else
    {
        httpd.response_404();
    }
}

static void do_httpd_serve(zcc::stream &fp)
{
    my_httpd *httpd = new my_httpd(fp);
    httpd->set_enable_form_data();
    while (1)
    {
        do_httpd_serve_once(*httpd);
        if (!httpd->maybe_continue())
        {
            break;
        }
        if (zcc::var_memleak_check_enable && zcc::var_sigint_flag)
        {
            break;
        }
    }
    delete httpd;
}