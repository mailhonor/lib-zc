#include "zc.h"

int zssl_INIT(int unused_flags)
{
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();

	return 0;
}

ZSSL_CTX *zssl_ctx_server_create(int unused_flags)
{
	ZSSL_CTX *zctx = 0;
	SSL_CTX *ctx = 0;

	ctx = SSL_CTX_new(SSLv23_server_method());
	if (!ctx) {
		return 0;
	}

	zctx = (ZSSL_CTX *) zcalloc(1, sizeof(ZSSL_CTX));
	zctx->ssl_ctx = ctx;
	zctx->server_or_client = 1;

	return zctx;

}

ZSSL_CTX *zssl_ctx_client_create(int unused_flags)
{
	ZSSL_CTX *zctx = 0;
	SSL_CTX *ctx = 0;

	ctx = SSL_CTX_new(SSLv23_client_method());
	if (!ctx) {
		return 0;
	}

	zctx = (ZSSL_CTX *) zcalloc(1, sizeof(ZSSL_CTX));
	zctx->ssl_ctx = ctx;
	zctx->server_or_client = 0;

	return zctx;

}

int zssl_ctx_set_cert(ZSSL_CTX * ssl_ctx, char *cert_file, char *key_file)
{
	ERR_clear_error();
	if ((!cert_file) || (SSL_CTX_use_certificate_chain_file(ssl_ctx->ssl_ctx, cert_file) <= 0)) {
		return (-1);
	}
	if ((!key_file) || (SSL_CTX_use_PrivateKey_file(ssl_ctx->ssl_ctx, key_file, SSL_FILETYPE_PEM) <= 0)) {
		return (-1);
	}
	if (!SSL_CTX_check_private_key(ssl_ctx->ssl_ctx)) {
		return (-1);
	}

	return 0;
}

void zssl_ctx_free(ZSSL_CTX * ctx)
{
	if (ctx->ssl_ctx) {
		SSL_CTX_free(ctx->ssl_ctx);
	}
	zfree(ctx);
}

void zssl_get_error(unsigned long *ecode, char *buf, int buf_len)
{
	unsigned long ec;
	ec = ERR_get_error();
	if (ecode) {
		*ecode = ec;
	}

	if (buf) {
		ERR_error_string_n(ec, buf, buf_len);
	}
}

ZSSL *zssl_create(ZSSL_CTX * ctx, int fd)
{
	ZSSL *zssl;
	SSL *ssl;

	ssl = SSL_new(ctx->ssl_ctx);
	SSL_set_fd(ssl, fd);

	zssl = (ZSSL *) zcalloc(1, sizeof(ZSSL));
	zssl->ssl = ssl;
	zssl->fd = fd;
	zssl->server_or_client = ctx->server_or_client;

	return zssl;
}

SSL *zssl_detach_ssl(ZSSL * zssl)
{
	SSL *ssl;

	ssl = zssl->ssl;

	zssl->ssl = 0;

	return ssl;
}

void zssl_free(ZSSL * ssl)
{
	if (ssl->ssl) {
		SSL_shutdown(ssl->ssl);
		SSL_free(ssl->ssl);
	}
	zfree(ssl);
}
