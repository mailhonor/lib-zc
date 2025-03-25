
[C++版本](./dns_cpp.md)


## 常用 DNS/IP地址/MAC地址 函数, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 封装了常用 DNS/IP地址/MAC地址 函数

_PS: 本文的ip地址都是 ipv4_

## 查询域名

### int zget_localaddr(zargv_t *addrs);

* 获取本机的 ip 地址, 追加到 addrs
* 返回 ip 地址个数

### int zget_hostaddr(const char *host, zargv_t *addrs);

* 获取域名的 ip 地址, 追加到 addrs
* 返回 ip 地址个数

## IP地址属性

### char *zget_ipstring(int ip, char *ipstr);

* ip(int类型, struct in_addr)转换为字符串, 结果存储在ipstr, 并返回

### int zget_ipint(const char *ipstr);

* ip 地址转 int
* zget_ipstring 的反操作

### int zget_network(int ip, int masklen);

* 返回 ip 的网络地址
* masklen是掩码长度(下同)

### int zget_netmask(int masklen);

* 返回子网掩码

### int zget_broadcast(int ip, int masklen);

* 返回 ip 的广播地址

### int zget_ip_min(int ip, int masklen);

* 返回 ip 所在网段的最小地址

### int zget_ip_max(int ip, int masklen);

* 返回 ip 所在网段的最大地址

### int zip_is_intranet(int ip);<BR />int zip_is_intranet2(const char *ip);

* 是否是保留地址
* 127.0.0.0/8, 10.0.0.0/8, 192.168.0.0/16, 172.16.0.0~172.31.255.255

## MAC地址

### int zget_mac_address(zargv_t *mac_list);

* 获取 MAC 地址, 追加到 mac_list
* 返回 mac 地址个数

## 例子: 域名查询

* [goto](../blob/master/sample/stdlib/get_addr.c)

## 例子: IP 属性

* [goto](../blob/master/sample/stdlib/ip.c)

## 例子: MAC

* [goto](../blob/master/sample/stdlib/mac.c)

