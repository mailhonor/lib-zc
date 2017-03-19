#include "zc.h"
#include "test_lib.h"
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "USAGE: %s www.somedomain.com\n", argv[0]);
		exit(2);
	}

	int sockfd, len;
	char *domain = argv[1];
	char buffer[102400];
	ZSSL_CTX *ctx;
	ZSSL *ssl;

	sockfd = zsocket_inet_connect_host(domain, 443, 10000);
	if (sockfd < 0) {
		fprintf(stderr, "can not connect (%s)\n", domain);
		exit(1);
	}

	zssl_INIT(0);
	ctx = zssl_ctx_client_create(0);
	ssl = zssl_create(ctx, sockfd);
	if (zssl_connect(ssl, 10000) < 0) {
		zssl_get_error(0, buffer, 1024);
		fprintf(stderr, "ssl error: %s\n", buffer);
		exit(1);
	}

	sprintf(buffer, "GET / HTTP/1.1\r\n" "Accept:*/*\r\n" "Accept-Language:en,en-us,zh-cn\r\n" "Accept-Encoding:gzip,deflate\r\n" "User-Agent:Mozilla/4.0(compatible;MSIE6.0;Windows NT 5.0)\r\n" "Host:%s\r\n" "\r\n", domain);
	if (zssl_write(ssl, buffer, strlen(buffer), 10000) < 1) {
		zssl_get_error(0, buffer, 1024);
		fprintf(stderr, "write error: %s\n", buffer);
		exit(0);
	}

	len = zssl_read(ssl, buffer, 102400, 10000);
	if (len < 0) {
		zssl_get_error(0, buffer, 1024);
		fprintf(stderr, "read error: %s\n", buffer);
	} else if (len == 0) {
		fprintf(stderr, "closed\n");
	} else {
		buffer[len] = 0;
		printf("read: %d\n%s\n", len, buffer);
	}

	zssl_free(ssl);
	zssl_ctx_free(ctx);
	close(sockfd);

	return 0;
}
