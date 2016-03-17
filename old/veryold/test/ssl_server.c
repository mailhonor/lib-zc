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
	int sockfd, fd, len;
	int port;
	char buffer[102400];
	ZSSL_CTX *ctx;
	ZSSL *ssl;

	if (argc != 2) {
		port = 443;
	} else {
		port = atoi(argv[1]);
	}

	sockfd = zsocket_inet_listen(0, port, 10000);
	if (sockfd < 0) {
		fprintf(stderr, "can not listen on  (%d)\n", port);
		exit(1);
	}

	zssl_INIT(0);
	ctx = zssl_ctx_server_create(0);
	if (zssl_ctx_set_cert(ctx, "/tmp/SSL.cert", "/tmp/SSL.key") < 0) {
		zssl_get_error(0, buffer, 1024);
		fprintf(stderr, "write error: %s\n", buffer);
	}
	while (1) {
		fd = zsocket_inet_accept(sockfd);
		if (fd < 0) {
			continue;
		}
		ssl = zssl_create(ctx, fd);
		if (zssl_accept(ssl, 10000) < 0) {
			goto while_err;
		}
		while (1) {
			len = zssl_read(ssl, buffer, 102400, 10000);
			if (len < 0) {
				zssl_get_error(0, buffer, 1024);
				fprintf(stderr, "read error: %s\n", buffer);
				goto while_err;
			} else if (len == 0) {
				fprintf(stderr, "closed\n");
				goto while_err;
			} else {
				buffer[len] = 0;
				printf("read: %d\n%s\n", len, buffer);
			}
			if (!strncasecmp(buffer, "exit", 4)) {
				break;
			}

			sprintf(buffer, "read somethin\n");
			if (zssl_write(ssl, buffer, strlen(buffer), 10000) < 1) {
				zssl_get_error(0, buffer, 1024);
				fprintf(stderr, "write error: %s\n", buffer);
				goto while_err;
			}
		}

	      while_err:
		zssl_free(ssl);
		close(fd);
	}

	zssl_ctx_free(ctx);
	close(sockfd);

	return 0;
}
