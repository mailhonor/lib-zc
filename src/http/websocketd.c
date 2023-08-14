/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2023-05-27
 * ================================
 */

#ifdef __linux__

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
// SHA1_Init

// https://developer.mozilla.org/zh-CN/docs/Web/API/WebSockets_API/Writing_WebSocket_servers

/*
Frame format:

      0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-------+-+-------------+-------------------------------+
     |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
     |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     | |1|2|3|       |K|             |                               |
     +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
     |     Extended payload length continued, if payload len == 127  |
     + - - - - - - - - - - - - - - - +-------------------------------+
     |                               |Masking-key, if MASK set to 1  |
     +-------------------------------+-------------------------------+
     | Masking-key (continued)       |          Payload Data         |
     +-------------------------------- - - - - - - - - - - - - - - - +
     :                     Payload Data continued ...                :
     + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
     |                     Payload Data continued ...                |
     +---------------------------------------------------------------+

DECODE:
    var DECODED = "";
    for (var i = 0; i < ENCODED.length; i++) {
        DECODED[i] = ENCODED[i] ^ MASK[i % 4];
    }
*/

#include "httpd.h"
#include <openssl/sha.h>

// 握手

zbool_t zhttpd_is_websocket(zhttpd_t *httpd)
{
    const char *v;
    v = zdict_get_str(httpd->request_headers, "upgrade", "");
    if (!ZSTR_EQ(v, "websocket"))
    {
        return 0;
    }
    return 1;
}

int zhttp_get_websocket_version(zhttpd_t *httpd)
{
    return zdict_get_int(httpd->request_headers, "sec-websocket-version", -1);
}

const char *zhttp_get_websocket_key(zhttpd_t *httpd)
{
    return zdict_get_str(httpd->request_headers, "sec-websocket-key", "");
}

int zhttpd_websocket_shakehand(zhttpd_t *httpd)
{
    // build Sec-WebSocket-Accept
    const char *v;
    v = zdict_get_str(httpd->request_headers, "sec-websocket-key", "");
    if (zempty(v))
    {
        return -1;
    }
    char rawkey[20 + 1];
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, v, strlen(v));
    SHA1_Update(&ctx, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
    SHA1_Final((unsigned char *)rawkey, &ctx);

    ZSTACK_BUF(key, 64);
    zbase64_encode(rawkey, 20, key, 0);

    // 输出 http 头
    zhttpd_response_header_initialization(httpd, httpd->version, "101 Switching Protocols");
    zhttpd_response_header(httpd, "Upgrade", "websocket");
    zhttpd_response_header(httpd, "Connection", "Upgrade");
    zhttpd_response_header(httpd, "Sec-WebSocket-Version", "13");
    zhttpd_response_header(httpd, "Sec-WebSocket-Accept", zbuf_data(key));
    zhttpd_response_header_over(httpd);
    if (zhttpd_response_flush(httpd) < 1)
    {
        return -1;
    }

    //
    return 1;
}

// 数据通信协议
struct zwebsocketd_t
{
    // read protocol
    char fin : 2;
    char rsv1 : 2;
    char rsv2 : 2;
    char rsv3 : 2;
    char opcode : 5;
    char mask : 2;
    char opcode_for_continue : 5;
    long payload_len;
    unsigned char masking_key[4];
    //
    long readed_len;
    //
    zstream_t *fp;
};

#define zvar_websocketd_type_continue 0X00
#define zvar_websocketd_type_text 0X01
#define zvar_websocketd_type_binary 0X02
#define zvar_websocketd_type_close 0X08
#define zvar_websocketd_type_ping 0X09
#define zvar_websocketd_type_pong 0X0A

zwebsocketd_t *zwebsocketd_open(zstream_t *fp)
{
    zwebsocketd_t *ws = (zwebsocketd_t *)zcalloc(1, sizeof(zwebsocketd_t));
    ws->fp = fp;
    return ws;
}

void zwebsocketd_close(zwebsocketd_t *ws, zbool_t close_fd_and_release_ssl)
{
    if (ws->fp)
    {
        zstream_close(ws->fp, close_fd_and_release_ssl);
        ws->fp = 0;
    }
    zfree(ws);
}

zbool_t zwebsocketd_get_header_pin(zwebsocketd_t *ws)
{
    return ws->fin ? 1 : 0;
}

int zwebsocketd_get_header_opcode(zwebsocketd_t *ws)
{
    return ws->opcode;
}

int zwebsocketd_read_frame_header(zwebsocketd_t *ws)
{
    unsigned char buf[128 + 1];
    unsigned char ch1, ch2;
    int pcount, i;
    zstream_t *fp = ws->fp;

    //
    if (zstream_readn_to_mem(ws->fp, buf, 2) != 2)
    {
        return -1;
    }
    ch1 = buf[0];
    ch2 = buf[1];

    ws->fin = (ch1 >> 7) & 0X01;
    ws->rsv1 = (ch1 >> 6) & 0X01;
    ws->rsv2 = (ch1 >> 5) & 0X01;
    ws->rsv3 = (ch1 >> 4) & 0X01;
    ws->opcode = ch1 & 0X0F;
    ws->mask = (ch2 >> 7) & 0X01;
    ws->payload_len = ch2 & 0X7F;

    ws->readed_len = 0;

    if (ws->opcode)
    {
        ws->opcode_for_continue = ws->opcode;
    }
    else
    {
        ws->opcode = ws->opcode_for_continue;
    }

    // payload_len
    if (ws->payload_len == 126)
    {
        pcount = 2;
    }
    else if (ws->payload_len == 127)
    {
        pcount = 8;
    }
    else
    {
        pcount = 0;
    }
    if (pcount > 0)
    {
        if (zstream_readn_to_mem(ws->fp, buf, pcount) != pcount)
        {
            return -1;
        }
        ws->payload_len = 0;
        for (i = 0; i < pcount; i++)
        {
            ws->payload_len = (ws->payload_len * 256) + buf[i];
        }
    }

    // masking_key
    if (ws->mask == 1)
    {
        if (zstream_readn_to_mem(ws->fp, ws->masking_key, 4) != 4)
        {
            return -1;
        }
    }

    if (ws->mask == 0)
    {
        return 0;
    }

    return 1;
}

int zwebsocketd_read_frame_data(zwebsocketd_t *ws, void *data, int len)
{
    if (ws->readed_len >= ws->payload_len)
    {
        return 0;
    }
    if (ws->payload_len - ws->readed_len < len)
    {
        len = ws->payload_len - ws->readed_len;
    }

    int r = zstream_readn_to_mem(ws->fp, data, len);
    if (r > 0)
    {
        if (ws->mask)
        {
            unsigned char *ps = (unsigned char *)data;
            unsigned char *mk = (unsigned char *)(&(ws->masking_key));
            for (int i = 0; i < r; i++)
            {
                ps[i] ^= mk[(ws->readed_len + i) % 4];
            }
        }
        ws->readed_len += r;
    }

    return r;
}

int zwebsocketd_write_frame_head_with_flags(zwebsocketd_t *ws, int len, zbool_t pin_flag, int opcode)
{
    unsigned char header[10 + 1];
    int hlen = 0;

    if (len < 0)
    {
        return -1;
    }

    memset(header, 0, sizeof(header));

    // fin
    header[0] = header[0] | 0X80;
    // opcode
    header[0] = header[0] | (opcode & 0X0F);

    if (len < 126)
    {
        hlen = 2;
        header[1] = len & 0X7F;
    }
    else if (len < 0X010000)
    {
        header[1] = 126;
        hlen = 4;
    }
    else
    {
        header[1] = 127;
        hlen = 10;
    }

    if (hlen > 2)
    {
        unsigned char *end = header + hlen;
        unsigned int tmplen = len;
        for (int i = (hlen == 4 ? 2 : 4); i > 0; i--)
        {
            end[-1] = tmplen & 0XFF;
            tmplen = (tmplen >> 8);
            end--;
        }
    }

    zstream_write(ws->fp, header, hlen);
    if (zstream_flush(ws->fp) < 1)
    {
        return -1;
    }
    return 1;
}

int zwebsocketd_write_frame_head_text(zwebsocketd_t *ws, int len)
{
    return zwebsocketd_write_frame_head_with_flags(ws, len, 1, zvar_websocketd_type_text);
}

int zwebsocketd_write_frame_head_binary(zwebsocketd_t *ws, int len)
{
    return zwebsocketd_write_frame_head_with_flags(ws, len, 1, zvar_websocketd_type_binary);
}

int zwebsocketd_write_frame_data(zwebsocketd_t *ws, const void *data, int len)
{
    if (len < 0)
    {
        len = strlen((const char *)data);
    }
    return zstream_write(ws->fp, data, len);
}

int zwebsocketd_write_frame_flush(zwebsocketd_t *ws)
{
    return zstream_flush(ws->fp);
}

static int zwebsocketd_send_data_with_opcode(zwebsocketd_t *ws, const void *data, int len, int opcode)
{
    if (len < 0)
    {
        len = strlen((const char *)data);
    }
    if (len > 125)
    {
        if ((opcode == zvar_websocketd_type_ping) || (opcode == zvar_websocketd_type_ping))
        {
            return 0;
        }
    }
    if (zwebsocketd_write_frame_head_with_flags(ws, len, 1, opcode) < 1)
    {
        return -1;
    }
    zwebsocketd_write_frame_data(ws, data, len);
    return zwebsocketd_write_frame_flush(ws);
}

int zwebsocketd_send_ping(zwebsocketd_t *ws, const void *data, int len)
{
    return zwebsocketd_send_data_with_opcode(ws, data, len, zvar_websocketd_type_ping);
}

int zwebsocketd_send_pong(zwebsocketd_t *ws, const void *data, int len)
{
    return zwebsocketd_send_data_with_opcode(ws, data, len, zvar_websocketd_type_pong);
}

int zwebsocketd_send_binary(zwebsocketd_t *ws, const void *data, int len)
{
    return zwebsocketd_send_data_with_opcode(ws, data, len, zvar_websocketd_type_binary);
}

int zwebsocketd_send_text(zwebsocketd_t *ws, const void *data, int len)
{
    return zwebsocketd_send_data_with_opcode(ws, data, len, zvar_websocketd_type_text);
}

int zwebsocketd_send_binary_printf_1024(zwebsocketd_t *ws, const char *format, ...)
{
    va_list ap;
    char buf[1024 + 1];
    int len;

    va_start(ap, format);
    len = zvsnprintf(buf, 1024, format, ap);
    len = ((len < 1024) ? len : (1024 - 1));
    va_end(ap);

    return zwebsocketd_send_data_with_opcode(ws, buf, len, zvar_websocketd_type_binary);
}

int zwebsocketd_send_text_printf_1024(zwebsocketd_t *ws, const char *format, ...)
{
    va_list ap;
    char buf[1024 + 1];
    int len;

    va_start(ap, format);
    len = zvsnprintf(buf, 1024, format, ap);
    len = ((len < 1024) ? len : (1024 - 1));
    va_end(ap);

    return zwebsocketd_send_data_with_opcode(ws, buf, len, zvar_websocketd_type_text);
}

#endif

