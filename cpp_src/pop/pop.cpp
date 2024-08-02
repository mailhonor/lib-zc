/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2024-01-30
 * ================================
 */

#include "./pop.h"

zcc_namespace_begin;

pop_client::pop_client()
{
}

pop_client::~pop_client()
{
}

void pop_client::set_ssl_tls(SSL_CTX *ssl_ctx, bool ssl_mode, bool tls_mode, bool tls_try_mode)
{
    ssl_mode_ = ssl_mode;
    tls_mode_ = tls_mode;
    ssl_ctx_ = ssl_ctx;
    tls_try_mode_ = tls_try_mode;
}

void pop_client::set_timeout(int timeout)
{
    fp_.set_timeout(timeout);
}

pop_client &pop_client::fp_append(const char *s, int slen)
{
    if (slen < 0)
    {
        slen = std::strlen(s);
    }
    fp_.append(s, slen);
    if (debug_protocol_fn_)
    {
        debug_protocol_fn_('C', s, slen);
    }
    return *this;
}

pop_client &pop_client::fp_append(const std::string &s)
{
    fp_.append(s);
    if (debug_protocol_fn_)
    {
        debug_protocol_fn_('C', s.c_str(), (int)s.size());
    }
    return *this;
}

int pop_client::fp_readn(void *mem, int strict_len)
{
    int r = fp_.readn(mem, strict_len);
    if (r < strict_len)
    {
        need_close_connection_ = true;
        return -1;
    }

    if (r > 0)
    {
        if (debug_protocol_fn_)
        {
            debug_protocol_fn_('S', (const char *)mem, r);
        }
    }
    return r;
}

int pop_client::fp_readn(std::string &str, int strict_len)
{
    int r = fp_.readn(str, strict_len);
    if (r < strict_len)
    {
        need_close_connection_ = true;
        return -1;
    }
    if (r > 0)
    {
        if (debug_protocol_fn_)
        {
            debug_protocol_fn_('S', str.c_str(), str.size());
        }
    }
    return r;
}

int pop_client::fp_read_delimiter(void *mem, int delimiter, int max_len)
{
    int r = fp_.read_delimiter(mem, delimiter, max_len);
    if (r < 0)
    {
        need_close_connection_ = true;
        return -1;
    }
    if (r > 0)
    {
        if (debug_protocol_fn_)
        {
            debug_protocol_fn_('S', (const char *)mem, r);
        }
    }
    return r;
}

int pop_client::fp_read_delimiter(std::string &str, int delimiter, int max_len)
{
    int r = fp_.read_delimiter(str, delimiter, max_len);
    if (r < 0)
    {
        need_close_connection_ = true;
        return -1;
    }
    if (r > 0)
    {
        if (debug_protocol_fn_)
        {
            debug_protocol_fn_('S', str.c_str(), str.size());
        }
    }
    return r;
}

int pop_client::simple_quick_cmd(const std::string &cmd)
{
    std::string response;
    return simple_quick_cmd(cmd, response);
}

int pop_client::simple_quick_cmd(const std::string &cmd, std::string &response)
{
    if (need_close_connection_)
    {
        return -1;
    }
    fp_append(cmd).fp_append("\r\n");
    if (fp_gets(response, 1024) < 1)
    {
        return -1;
    }
    if (response[0] != '+')
    {
        return 0;
    }
    return 1;
}

int pop_client::do_STLS()
{
    if (need_close_connection_)
    {
        return -1;
    }
    int r;
    if (ssl_flag_)
    {
        return 1;
    }

    if ((r = simple_quick_cmd("STLS")) < 1)
    {
        return r;
    }

    if (fp_.tls_connect(ssl_ctx_) < 0)
    {
        zcc_pop_client_error("建立SSL");
        return -1;
    }
    ssl_flag_ = true;

    return 1;
}

int pop_client::fp_connect(const char *destination, int times)
{
    if (need_close_connection_)
    {
        return -1;
    }
    if (connected_)
    {
        return 1;
    }
    if (times > 0)
    {
        for (int i = 0; i < times; i++)
        {
            if (fp_connect(destination, 0) > 0)
            {
                return 1;
            }
        }
        return -1;
    }
    if (!fp_.connect(destination))
    {
        zcc_pop_client_error("连接(%s)", destination);
        need_close_connection_ = true;
        return -1;
    }
    if (ssl_mode_)
    {
        if (fp_.tls_connect(ssl_ctx_) < 0)
        {
            disconnect();
            need_close_connection_ = true;
            zcc_pop_client_error("建立SSL(%s)", destination);
            return -1;
        }
        ssl_flag_ = true;
    }
    need_close_connection_ = false;
    connected_ = true;
    return 1;
}

void pop_client::disconnect()
{
    fp_.close();
    opened_ = false;
    need_close_connection_ = false;
    connected_ = false;
    authed_ = false;
    quited_ = false;
    ssl_flag_ = false;
    capa_before_auth_ = -1;
    capa_after_auth_ = -1;
}

int pop_client::welcome()
{
    std::string line;
    if (fp_gets(line, 1024) < 1)
    {
        zcc_pop_client_info("错误: pop 读取 welcome 失败");
        return -1;
    }
    zcc_pop_client_debug("pop 读: %s", line.c_str());
    if (line[0] != '+')
    {
        return 0;
    }
    zcc::trim_line_end_rn(line);
    if (line[line.size() - 1] == '>')
    {
        auto pos = line.find_last_of('<');
        if (pos == std::string::npos)
        {
            return 1;
        }
        banner_apop_id_ = line.substr(pos);
    }
    return 1;
}

int pop_client::connect(const char *destination, int times)
{
    int r = -1;
    if (opened_)
    {
        return 1;
    }
    if (times < 1)
    {
        times = 1;
    }

    if ((r = fp_connect(destination, 3)) < 1)
    {
        return r;
    }

    if ((r = welcome()) < 1)
    {
        return r;
    }

    if (tls_mode_)
    {
        if ((r = do_STLS()) < 1)
        {
            if (r < 0)
            {
                return r;
            }
            if (!tls_try_mode_)
            {
                return -1;
            }
        }
    }

    return 1;
}

int pop_client::cmd_quit()
{
    if (quited_)
    {
        return 1;
    }
    if (need_close_connection_)
    {
        return 1;
    }
    quited_ = true;
    return simple_quick_cmd("QUIT");
}

int pop_client::auth_basic(const char *user, const char *password)
{
    std::string cmd;
    int r;

    if (need_close_connection_)
    {
        return -1;
    }

    if (empty(user))
    {
        return 1;
    }

    cmd.clear();
    cmd.append("USER ").append(user);
    if ((r = simple_quick_cmd(cmd)) < 1)
    {
        return r;
    }

    cmd.clear();
    cmd.append("PASS ").append(password);
    if ((r = simple_quick_cmd(cmd)) < 1)
    {
        return r;
    }

    authed_ = true;
    return r;
}

int pop_client::auth_apop(const char *user, const char *password)
{
    std::string cmd;
    int r;

    if (need_close_connection_)
    {
        return -1;
    }

    if (empty(user))
    {
        return 1;
    }

    if (banner_apop_id_.empty())
    {
        return 1;
    }

    cmd = banner_apop_id_;
    cmd.append(password);

    cmd.clear();
    cmd.append("APOP ").append(user).append(" ");
    cmd.append(md5(cmd));
    if ((r = simple_quick_cmd(cmd)) < 1)
    {
        return r;
    }

    authed_ = true;
    return r;
}

int pop_client::auth_auto(const char *user, const char *password)
{
    int r = 0;
    if (banner_apop_id_.size() > 1)
    {
        if ((r = auth_apop(user, password)) < 0)
        {
            return r;
        }
        if (r > 0)
        {
            return r;
        }
    }
    return auth_basic(user, password);
}

int pop_client::cmd_capa(std::vector<std::string> &capability)
{
    std::string line;
    int r;
    if (need_close_connection_)
    {
        return -1;
    }
    r = simple_quick_cmd("CAPA");
    while (r > 0)
    {
        line.clear();
        if (fp_gets(line, 1024) < 1)
        {
            r = -1;
            break;
        }
        if ((line == ".\r\n") || (line == ".\n"))
        {
            break;
        }
        zcc::tolower(line);
        capability.push_back(line);
    }
    return r;
}

int pop_client::cmd_capa()
{

    if ((capa_before_auth_ != -1) && (!authed_))
    {
        return capa_before_auth_;
    }
    if ((capa_after_auth_ != -1) && authed_)
    {
        return capa_after_auth_;
    }

    capability_.clear();
    int r = cmd_capa(capability_);
    if (!authed_)
    {
        capa_before_auth_ = r;
    }
    else
    {
        capa_after_auth_ = r;
    }
    return r;
}

const std::string &pop_client::get_capability(const char *key_lowercase)
{
    cmd_capa();
    std::string key;
    key.append(key_lowercase);
    zcc::tolower(key);
    key.append(" ");
    for (auto it = capability_.begin(); it != capability_.end(); it++)
    {
        if (!it->compare(0, key.size(), key))
        {
            return *it;
        }
    }
    return blank_;
}

const std::vector<std::string> &pop_client::get_capability()
{
    cmd_capa();
    return capability_;
}

int pop_client::cmd_stat(uint64_t &count, uint64_t &size_sum)
{
    if (need_close_connection_)
    {
        return -1;
    }
    std::string response;
    int r = simple_quick_cmd("STAT");
    if (r < 1)
    {
        return r;
    }
    const char *ps = std::strchr(response.c_str(), ' ');
    if (!ps)
    {
        return 0;
    }
    count = atoi(ps + 1);
    ps = std::strchr(ps + 1, ' ');
    if (!ps)
    {
        return 0;
    }
    size_sum = atol(ps + 1);
    return 1;
}

int pop_client::cmd_list(std::vector<std::pair<int, uint64_t>> &msg_number_sizes)
{
    if (need_close_connection_)
    {
        return -1;
    }
    std::string line;
    int r = simple_quick_cmd("LIST");
    int mn;
    uint64_t size;

    while (r > 0)
    {
        line.clear();
        mn = 0;
        size = 0;
        if (fp_gets(line, 1024) < 1)
        {
            r = -1;
            break;
        }
        if (line == ".")
        {
            break;
        }
        const char *ps = line.c_str();
        mn = atoi(ps);
        ps = std::strchr(ps, ' ');
        if (!ps)
        {
            size = atol(ps + 1);
        }
        msg_number_sizes.push_back(std::make_pair(mn, size));
    }
    return r;
}

int pop_client::cmd_list(std::vector<int> &msg_numbers)
{
    if (need_close_connection_)
    {
        return -1;
    }
    std::string line;
    int r = simple_quick_cmd("LIST");
    int mn;

    while (r > 0)
    {
        line.clear();
        mn = 0;
        if (fp_gets(line, 1024) < 1)
        {
            r = -1;
            break;
        }
        if ((line == ".\r\n") || (line == ".\n"))
        {
            break;
        }
        const char *ps = line.c_str();
        mn = atoi(ps);
        msg_numbers.push_back(mn);
    }
    return r;
}

int pop_client::cmd_list(int msg_number, uint64_t &size)
{
    if (need_close_connection_)
    {
        return -1;
    }
    std::string cmd;
    std::string line;
    int r;

    cmd.append("LIST ").append(std::to_string(msg_number));
    if ((r = simple_quick_cmd(cmd, line)) < 1)
    {
        return r;
    }
    auto pos = line.find_last_of(' ');
    if (pos == std::string::npos)
    {
        return -1;
    }
    size = atol(line.c_str() + pos + 1);
    return r;
}

int pop_client::cmd_uidl(std::map<std::string, int> &result)
{
    if (need_close_connection_)
    {
        return -1;
    }
    std::string line;
    int r = simple_quick_cmd("UIDL");
    int mn;
    uint64_t size;

    while (r > 0)
    {
        mn = 0;
        size = 0;
        if (fp_gets(line, 1024) < 1)
        {
            r = -1;
            break;
        }
        trim_line_end_rn(line);
        if (line == ".")
        {
            break;
        }
        const char *ps = std::strchr(line.c_str(), ' ');
        if (!ps)
        {
            continue;
        }
        result[ps + 1] = atoi(line.c_str());
    }
    return r;
}

int pop_client::cmd_uidl(int msg_number, std::string &unique_id)
{
    if (need_close_connection_)
    {
        return -1;
    }
    std::string cmd;
    std::string line;
    int r;

    cmd.append("UIDL ").append(std::to_string(msg_number));
    if ((r = simple_quick_cmd(cmd, line)) < 1)
    {
        return r;
    }
    auto pos = line.find_last_of(' ');
    if (pos == std::string::npos)
    {
        return -1;
    }
    unique_id = line.c_str() + pos + 1;
    return r;
}

int pop_client::_get_msg_data(const std::string &cmd, FILE *fp, std::string *data)
{
    if (need_close_connection_)
    {
        return -1;
    }
    std::string line;
    int r;

    if ((r = simple_quick_cmd(cmd, line)) < 1)
    {
        return r;
    }
    bool last_line_over = true;
    while (1)
    {
        line.clear();
        int skip_dot_length = 0;
        int ret = fp_gets(line, 10240);
        if (r < 1)
        {
            r = -1;
            break;
        }
        if (last_line_over)
        {
            if ((line == ".\r\n") || (line == ".\n"))
            {
                break;
            }
            if (line[0] == '.')
            {
                skip_dot_length = 1;
            }
        }
        if (fp)
        {
            fwrite(line.c_str() + skip_dot_length, 1, line.size() - skip_dot_length, fp);
        }
        if (data)
        {
            data->append(line.c_str() + skip_dot_length, line.size() - skip_dot_length);
        }

        if (line[ret - 1] == '\n')
        {
            last_line_over = true;
        }
        else
        {
            last_line_over = false;
        }
    }
    return r;
}

int pop_client::cmd_retr(int msg_number, std::string &data)
{
    if (need_close_connection_)
    {
        return -1;
    }
    std::string cmd;
    cmd.append("RETR ").append(std::to_string(msg_number));
    return _get_msg_data(cmd, 0, &data);
}

int pop_client::cmd_retr(int msg_number, FILE *fp)
{
    if (need_close_connection_)
    {
        return -1;
    }
    std::string cmd;
    cmd.append("RETR ").append(std::to_string(msg_number));
    return _get_msg_data(cmd, fp, 0);
}

int pop_client::cmd_top(int msg_number, std::string &data, int extra_line_count)
{
    if (need_close_connection_)
    {
        return -1;
    }
    std::string cmd;
    cmd.append("TOP ").append(std::to_string(msg_number)).append(" ").append(std::to_string(extra_line_count));
    return _get_msg_data(cmd, 0, &data);
}

int pop_client::cmd_dele(int msg_number)
{
    if (need_close_connection_)
    {
        return -1;
    }
    std::string cmd;
    cmd.append("DELE ").append(std::to_string(msg_number));
    return simple_quick_cmd(cmd);
}

int pop_client::cmd_noop()
{
    if (need_close_connection_)
    {
        return -1;
    }
    return simple_quick_cmd("NOOP");
}

int pop_client::cmd_rset()
{
    if (need_close_connection_)
    {
        return -1;
    }
    return simple_quick_cmd("RSET");
}

void pop_client::close()
{
    cmd_quit();
    disconnect();
}

zcc_namespace_end;
