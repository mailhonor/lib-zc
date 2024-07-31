/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2023-05-27
 * ================================
 */

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif // __GNUC__

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

#include "zcc/zcc_http.h"
#include <openssl/sha.h>

zcc_namespace_begin;

// 握手

bool httpd::is_websocket_upgrade()
{
    if (strcmp(get_cstring(request_headers_, "upgrade", ""), "websocket"))
    {
        return false;
    }
    return true;
}

int httpd::get_websocket_version()
{
    return get_int(request_headers_, "sec-websocket-version", -1);
}

const char *httpd::get_websocket_key()
{
    return get_cstring(request_headers_, "sec-websocket-key", "");
}

bool httpd::websocket_shakehand()
{
    // build Sec-WebSocket-Accept
    const char *v;
    v = get_cstring(request_headers_, "sec-websocket-key", "");
    if (zcc::empty(v))
    {
        return false;
    }
    char rawkey[20 + 1];
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, v, strlen(v));
    SHA1_Update(&ctx, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
    SHA1_Final((unsigned char *)rawkey, &ctx);

    std::string key;
    zcc::base64_encode(rawkey, 20, key);

    // 输出 http 头
    log_info("101 -");
    response_header_initialization("101 Switching Protocols");
    response_header("Upgrade", "websocket");
    response_header("Connection", "Upgrade");
    response_header("Sec-WebSocket-Version", "13");
    response_header("Sec-WebSocket-Accept", key);
    response_header_over();
    response_flush();

    return is_exception() ? false : true;
}

char websocketd::opcode_continue = 0X00;
char websocketd::opcode_text = 0X01;
char websocketd::opcode_binary = 0X02;
char websocketd::opcode_close = 0X08;
char websocketd::opcode_ping = 0X09;
char websocketd::opcode_pong = 0X0A;

websocketd::websocketd(stream &fp)
{
    fp_ = &fp;
}
websocketd::~websocketd()
{
    if (fp_)
    {
        delete fp_;
        fp_ = nullptr;
    }
}

bool websocketd::read_frame_header()
{
    unsigned char buf[128 + 1];
    unsigned char ch1, ch2;
    int pcount, i;

    //
    if (fp_->readn(buf, 2) != 2)
    {
        return false;
    }
    ch1 = buf[0];
    ch2 = buf[1];

    fin_ = (ch1 >> 7) & 0X01;
    rsv1_ = (ch1 >> 6) & 0X01;
    rsv2_ = (ch1 >> 5) & 0X01;
    rsv3_ = (ch1 >> 4) & 0X01;
    opcode_ = ch1 & 0X0F;
    mask_ = (ch2 >> 7) & 0X01;
    payload_len_ = ch2 & 0X7F;

    readed_len_ = 0;

    if (opcode_)
    {
        opcode_for_continue_ = opcode_;
    }
    else
    {
        opcode_ = opcode_for_continue_;
    }

    // payload_len
    if (payload_len_ == 126)
    {
        pcount = 2;
    }
    else if (payload_len_ == 127)
    {
        pcount = 8;
    }
    else
    {
        pcount = 0;
    }
    if (pcount > 0)
    {
        if (fp_->readn(buf, pcount) != pcount)
        {
            return false;
        }
        payload_len_ = 0;
        for (i = 0; i < pcount; i++)
        {
            payload_len_ = (payload_len_ * 256) + buf[i];
        }
    }

    // masking_key
    if (mask_)
    {
        if (fp_->readn(masking_key_, 4) != 4)
        {
            return false;
        }
    }

    if (!mask_)
    {
        return false;
    }

    return true;
}

int websocketd::read_frame_data(void *data, int len)
{
    if (readed_len_ >= payload_len_)
    {
        return 0;
    }
    if (payload_len_ - readed_len_ < len)
    {
        len = payload_len_ - readed_len_;
    }

    int r = fp_->readn(data, len);
    if (r > 0)
    {
        if (mask_)
        {
            unsigned char *ps = (unsigned char *)data;
            unsigned char *mk = (unsigned char *)(&(masking_key_));
            for (int i = 0; i < r; i++)
            {
                ps[i] ^= mk[(readed_len_ + i) % 4];
            }
        }
        readed_len_ += r;
    }

    return r;
}

int websocketd::read_frame_data(std::string &data, int len)
{
    data.clear();
    if (readed_len_ >= payload_len_)
    {
        return 0;
    }
    if (payload_len_ - readed_len_ < len)
    {
        len = payload_len_ - readed_len_;
    }

    int r = fp_->readn(data, len);
    if (r > 0)
    {
        if (mask_)
        {
            unsigned char *ps = (unsigned char *)data.c_str();
            unsigned char *mk = (unsigned char *)(&(masking_key_));
            for (int i = 0; i < r; i++)
            {
                data[i] = ((unsigned char )data[i]) ^ mk[(readed_len_ + i) % 4];
            }
        }
        readed_len_ += r;
    }

    return r;
}

bool websocketd::write_frame_head_with_flags(int len, bool fin_flag, int opcode)
{
    unsigned char header[10 + 1];
    int hlen = 0;

    if (len < 0)
    {
        return false;
    }

    std::memset(header, 0, sizeof(header) - 1);

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

    fp_->write(header, hlen);
    if (fp_->flush() < 1)
    {
        return false;
    }
    return true;
}

bool websocketd::write_frame_head_text(int len)
{
    return write_frame_head_with_flags(len, true, opcode_text);
}

bool websocketd::write_frame_head_binary(int len)
{
    return write_frame_head_with_flags(len, true, opcode_binary);
}

bool websocketd::write_frame_data(const void *data, int len)
{
    if (len < 0)
    {
        len = strlen((const char *)data);
    }
    return fp_->write(data, len) > 0;
}

bool websocketd::write_frame_flush()
{
    return fp_->flush() > 0;
}

bool websocketd::send_data_with_opcode(const void *data, int len, int opcode)
{
    if (len < 0)
    {
        len = std::strlen((const char *)data);
    }
    if (len > 125)
    {
        if ((opcode == opcode_ping) || (opcode == opcode_pong))
        {
            return true;
        }
    }
    if (!write_frame_head_with_flags(len, true, opcode))
    {
        return false;
    }
    write_frame_data(data, len);
    return write_frame_flush();
}

bool websocketd::send_ping(const void *data, int len)
{
    return send_data_with_opcode(data, len, opcode_ping);
}

bool websocketd::send_pong(const void *data, int len)
{
    return send_data_with_opcode(data, len, opcode_pong);
}

bool websocketd::send_text(const void *data, int len)
{
    return send_data_with_opcode(data, len, opcode_text);
}

bool websocketd::send_binary(const void *data, int len)
{
    return send_data_with_opcode(data, len, opcode_binary);
}

bool websocketd::send_text_printf_1024(const char *format, ...)
{
    va_list ap;
    char buf[1024 + 1];
    int len;

    va_start(ap, format);
    len = std::snprintf(buf, 1024, format, ap);
    len = ((len < 1024) ? len : (1024 - 1));
    va_end(ap);

    return send_data_with_opcode(buf, len, opcode_text);
}

zcc_namespace_end;
