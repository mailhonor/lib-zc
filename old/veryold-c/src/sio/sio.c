#include "zc.h"

ZSIO *zsio_create(int unused)
{
	ZSIO *fp;
	ZBUF *zb;

	fp = (ZSIO *) zcalloc(1, sizeof(ZSIO));

	zb = &(fp->rbuf);
	zb->data = fp->rbuf_data;
	zb->size = ZSIO_RBUF_SIZE;
	zb->ready = zsio_read_ready;
	zb->space = 0;
	ZBUF_RESET(zb);

	zb = &(fp->wbuf);
	zb->data = fp->wbuf_data;
	zb->size = ZSIO_WBUF_SIZE;
	zb->ready = zsio_write_ready;
	zb->space = 0;
	ZBUF_RESET(zb);

	fp->io_ctx = 0;

	fp->timeout = zmtime_set_timeout(24 * 3600 * 1000);

	return fp;
}

void *zsio_free(ZSIO * fp)
{
	void *r;

	ZSIO_FFLUSH(fp);
	r = fp->io_ctx;
	zfree(fp);

	return r;
}

void zsio_reset(ZSIO * fp)
{
	ZBUF_RESET((&(fp->rbuf)));
	ZBUF_RESET((&(fp->wbuf)));
	fp->io_ctx = 0;
	fp->read_fn = 0;
	fp->write_fn = 0;
}

int zsio_set_ioctx(ZSIO * fp, void *io_ctx, ZSIO_READ_FN read_fn, ZSIO_WRITE_FN write_fn)
{
	fp->io_ctx = io_ctx;
	fp->read_fn = read_fn;
	fp->write_fn = write_fn;

	return 0;
}

int zsio_set_timeout(ZSIO * fp, int timeout)
{
	fp->timeout = zmtime_set_timeout(timeout);

	return 0;
}

int zsio_read_ready(ZBUF * rbuf)
{
	int ret;
	ZSIO *fp;

	fp = ZCONTAINER_OF(rbuf, ZSIO, rbuf);

	ret = zsio_read_cache(fp);
	if (ret < 1) {
		return ZBUF_EOF;
	}

	return 0;

}

int zsio_write_ready(ZBUF * wbuf)
{
	int ret;
	ZSIO *fp;

	fp = ZCONTAINER_OF(wbuf, ZSIO, wbuf);
	ret = zsio_fflush(fp);
	if (zsio_fflush(fp) > -1) {
		return 0;
	}

	return ret;
}

int zsio_read_cache(ZSIO * fp)
{
	int ret;
	int time_left;
	ZBUF *rbuf;

	rbuf = &(fp->rbuf);
	if (rbuf->len > 0) {
		return rbuf->len;
	}

	ret = ZSIO_FFLUSH(fp);

	if (ret < 0) {
		return ret;
	}

	time_left = zmtime_left(fp->timeout);
	if (time_left < 1) {
		fp->flags |= ZSIO_FLAG_TIMEOUT;
		return ZSIO_ERROR;
	}
	ZBUF_RESET(rbuf);
	ret = fp->read_fn(fp, rbuf->data, ZSIO_RBUF_SIZE, time_left);
	if (ret == 0) {
		fp->flags |= ZSIO_FLAG_EOF;
		return ZSIO_EOF;
	}
	if (ret < 0) {
		fp->flags |= ZSIO_FLAG_ERR;
		return ZSIO_ERROR;
	}
	rbuf->len += ret;

	return ret;
}

int zsio_fflush(ZSIO * fp)
{
	int ret;
	int time_left;
	ZBUF *wbuf;
	char *data;
	int data_len;
	int rlen;

	wbuf = &(fp->wbuf);

	if (wbuf->len < 1) {
		return 0;
	}
	data = (char *)(wbuf->data);
	data_len = wbuf->len;
	rlen = 0;
	while (rlen < data_len) {
		time_left = zmtime_left(fp->timeout);
		if (time_left < 1) {
			fp->flags |= ZSIO_FLAG_TIMEOUT;
			return ZSIO_TIMEOUT;
		}
		ret = fp->write_fn(fp, data + rlen, data_len - rlen, time_left);
		if (ret < 1) {
			if (ret == ZIO_TIMEOUT) {
				fp->flags |= ZSIO_FLAG_TIMEOUT;
				return ZSIO_TIMEOUT;
			}
			fp->flags |= ZSIO_FLAG_ERR;
			return ZSIO_ERR;
		}
		rlen += ret;
	}
	ZBUF_RESET(wbuf);

	return rlen;
}
