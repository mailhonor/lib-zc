<A name="readme_md" id="readme_md"></A>

[C版本](./dns.md)


## 常用 DNS/IP地址/MAC地址 函数, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

```c++
// 获取域名/主机名的ip地址, 存储到addrs
int get_hostaddr(const char *host, std::vector<std::string> &addrs);
inline int get_hostaddr(const std::string &host, std::vector<std::string> &addrs)
{
    return get_hostaddr(host.c_str(), addrs);
}
// 获取本机ip地址
int get_localaddr(std::vector<std::string> &addrs);
// ip 地址转 int
int get_ipint(const char *ipstr);
inline int get_ipint(const std::string &ipstr)
{
    return get_ipint(ipstr.c_str());
}
// 获得ip地址的网段地址
int get_network(int ip, int masklen);
// 获得ip地址的掩码
int get_netmask(int masklen);
// 获得ip地址的广播地址
int get_broadcast(int ip, int masklen);
// 指定掩码, 最小的ip
int get_ip_min(int ip, int masklen);
// 指定掩码, 最大的ip
int get_ip_max(int ip, int masklen);
// 是不是 ip
bool is_ip(const char *ip);
inline bool is_ip(const std::string &ip)
{
    return is_ip(ip.c_str());
}
// 是否保留地址
int is_intranet(int ip);
int is_intranet2(const char *ip);
// ip 转字符串
char *get_ipstring(int ip, char *ipstr);
std::string get_ipstring(int ip);

// 获取本机mac地址
int get_mac_address(std::vector<std::string> &mac_list);
```