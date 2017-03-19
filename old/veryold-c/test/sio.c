#include "zc.h"
#include "test_lib.h"

int main(int argc, char **argv)
{
	int ret, fd, port = 25;
	char *dest = "nsmail_dev";
	char buf[1024];
	ZSIO *fp;
	int use_ssl = 0;
	ZSSL_CTX *ssl_ctx;
	ZSSL *ssl;

	if (argc > 1) {
		dest = argv[1];
	}
	if (argc > 2) {
		port = atoi(argv[2]);
	}
	if (port == 465) {
		use_ssl = 1;
	}

	fd = zsocket_inet_connect_host(dest, port, 10 * 1000);
	if (fd < 0) {
		zlog_info("connect: %m");
		exit(1);
	}

	fp = zsio_create(0);
	zsio_set_timeout(fp, 10 * 1000);

	if (use_ssl) {
		zssl_INIT(0);
		ssl_ctx = zssl_ctx_client_create(0);
		ssl = zssl_create(ssl_ctx, fd);
		if (zssl_connect(ssl, 10000) < 0) {
			zssl_get_error(0, buf, 1000);
			fprintf(stderr, "ssl error: %s\n", buf);
			exit(1);
		}
		zsio_set_SSL(fp, ssl);
	} else {
		zsio_set_FD(fp, fd);
	}

	ret = zsio_read_line(fp, buf, 1024);
	if (ret < 1) {
		zlog_info("read error:%d: %m", ret);
		exit(1);
	}

	buf[ret] = 0;
	zlog_info("%s", buf);

	zsio_fprintf(fp, "%s\r\n", "quit");
	ZSIO_FFLUSH(fp);

	while (1) {
		ret = zsio_read_line(fp, buf, 1024);
		if (ret < 1) {
			zlog_info("read error: %d", ret);
			exit(1);
		}

		buf[ret] = 0;
		zlog_info("last_buf: %s", buf);
	}

	return 0;
}
