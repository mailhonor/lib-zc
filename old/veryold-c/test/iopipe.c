#include "zc.h"
#include "test_lib.h"

ZIOPIPE_BASE *iopb;
pthread_t pth_t;
int sock, fd, rfd;

ZSSL_CTX *ssl_ctx;
ZSSL *ssl;

char *host;
int is_ssl;
int port;

static void *_run(void *arg)
{
	char buffer[10240];
	sock = zsocket_inet_listen(0, 99, -1);

	while (1) {
		ssl = 0;
		fd = zsocket_inet_accept(sock);
		if (fd < 0) {
			continue;
		}
		zio_nonblocking(fd, 1);

		rfd = zsocket_inet_connect_host(host, port, 10000);
		if (rfd < 0) {
			close(fd);
			continue;
		}
		zio_nonblocking(rfd, 1);
		SSL *s = 0;
		if (is_ssl) {
			ssl = zssl_create(ssl_ctx, rfd);
			if (zssl_connect(ssl, 10000) < 0) {
				zssl_get_error(0, buffer, 1024);
				fprintf(stderr, "ssl error: %s\n", buffer);
				exit(1);
			}
			s = zssl_detach_ssl(ssl);
			zssl_free(ssl);
		}

		ziopipe_enter(iopb, fd, 0, rfd, s);
	}

	return arg;
}

int main(int argc, char **argv)
{
	test_init(argc, argv);

	is_ssl = zdict_get_bool(test_argv_dict, "ssl", 0);
	host = zdict_get_str(test_argv_dict, "host", "localhost");
	port = zdict_get_int(test_argv_dict, "port", 0);
	if (port == 0) {
		printf("USAGE %s [-o ssl=yes] [-o host=127.0.0.1] [-port 25]\n", argv[0]);
		return 0;
	}

	if (is_ssl) {
		zssl_INIT(0);
		ssl_ctx = zssl_ctx_client_create(0);
	}
	iopb = ziopipe_base_create();
	_run(0);

	pthread_create(&pth_t, 0, _run, 0);

	while (1) {
		ziopipe_base_dispatch(iopb, 0);
	}

	return 0;
}
