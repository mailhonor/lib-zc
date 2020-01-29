# DNS / IP地址 / MAC地址

[LIB-ZC](https://gitee.com/linuxmail/lib-zc) 是一个C扩展库

内部封装了几个和dns,ip地址,mac地址相关的函数

PS: 下文的ip都是ipv4

## 函数

### 本机ip地址

```
/* 获取本机ip地址, 返回ip地址个数 */
int zget_localaddr(zargv_t *addrs);
```

### 域名ip地址

```
/* 获取host对应的ip地址, 返回ip地址个数 */
int zget_hostaddr(const char *host, zargv_t *addrs);

```

### socket另一端的ip和端口

```
/* 获取socket文件描述符sockfd,另一端的ip和端口信息; host: struct in_addr */
zbool_t zget_peername(int sockfd, int *host, int *port);
```

### ip(int类型)转字符换

```
/* ip(struct in_addr)转ip地址(1.1.1.1), 结果存储在ipstr, 并返回 */
char *zget_ipstring(int ip, char *ipstr);
```

### ip地址转int

```
/* zget_ipstring 的反操作 */
int zget_ipint(const char *ipstr);
```

### 网络地址

```
/* 返回ip的网络地址, masklen是掩码长度(下同) */
int zget_network(int ip, int masklen);
```

### 子网掩码

```
/* 返回子网掩码 */
int zget_netmask(int masklen);

```

### 广播地址

```
/* 返回ip的广播地址 */
int zget_broadcast(int ip, int masklen);
```

### 子网最新地址

```
/* 返回ip所在网段的最小地址 */
int zget_ip_min(int ip, int masklen);
```

### 子网最大地址

```
/* 返回ip所在网段的最大地址 */
int zget_ip_max(int ip, int masklen);
```

### 是否保留地址

包括 127.0.0.0/8, 10.0.0.0/8, 192.168.0.0/16, 172.16.0.0~172.31.255.255
```
/* 是否保留地址 */
int zip_is_intranet(int ip);

/* 是否保留地址 */
int zip_is_intranet2(const char *ip);
```

### 获取mac地址

```
/* 获取mac地址; 返回个数, -1: 错; src/stdlib/mac_address.c */
nt zget_mac_address(zargv_t *mac_list);
```

## 例子: 查询域名地址

```
#include "zc.h"

static void get_host_addr(char *host)
{
    zargv_t *addr_list =  zargv_create(0);
    int count;

    count = zget_hostaddr(host, addr_list);
    if (count == 0) {
        printf("%s'addr none\n", host);
    } else if (count < 0) {
        printf("%s'addr error\n", host);
    } else {
        printf("%s'addr list(%d):\n", host, count);
        ZARGV_WALK_BEGIN(addr_list, ip) {
            printf("    %s\n", ip);
        } ZARGV_WALK_END;
    }   
    zargv_free(addr_list);
}
int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("USAGE: %s host_or_domain\n", argv[0]);
        return 0;
    }
    get_host_addr(argv[1]);

    return 0
}
```

## 例子: ip

```
#include "zc.h"
int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("USAGE: %s ip [ mask-length ]\n", argv[0]);
        return 1;
    }   
    char *ipstr = argv[1];
    int masklen = 32,  ipint;
    if (argc > 2) {
        masklen = atoi(argv[2]);
    }   
    printf("输入: %s %d\n\n", ipstr, masklen);
    ipint = zget_ipint(ipstr);
    printf("ipint: %u\n", (unsigned int)ipint);
    printf("masklen: %d\n", masklen);

    if (masklen == -1) {
        return 0;
    }
    if ((masklen < 1) || (masklen > 32)) {
        printf("ERR unknown mask-length: %s\n", argv[2]);
        return 1;
    }

    char ipbuf[20];
    printf("网络地址: %s\n", zget_ipstring(zget_network(ipint, masklen), ipbuf));
    printf("子网掩码: %s\n", zget_ipstring(zget_netmask(masklen), ipbuf));
    printf("广播地址: %s\n", zget_ipstring(zget_broadcast(ipint, masklen), ipbuf));
    printf("最小地址: %s\n", zget_ipstring(zget_ip_min(ipint, masklen), ipbuf));
    printf("最大地址: %s\n", zget_ipstring(zget_ip_max(ipint, masklen), ipbuf));
    printf("保留地址: %s\n", zip_is_intranet(ipint)?"yes":"no");
    return 0;
}
```

## 例子: mac地址

```
#include "zc.h"

int main(int argc, char **argv)
{
    zargv_t *ms = zargv_create(-1);
    if (zget_mac_address(ms) < 0) {
        printf("ERR can not get mac address(%m)\n");
        zargv_free(ms);
        return 1;
    }   
    printf("found %d mac address\n", zargv_len(ms));
    ZARGV_WALK_BEGIN(ms, mac) {
        printf("%s\n", mac);
    } ZARGV_WALK_END;
    zargv_free(ms);
    return 0;
}
```
