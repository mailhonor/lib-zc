/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-11-08
 * ================================
 */

#include "zc.h"
#include <errno.h>

#define _iopipe_buffer_size 4096

typedef struct _iopipe_pair_t _iopipe_pair_t;
struct _iopipe_pair_t
{
    zaio_t *client;
    zaio_t *server;
    void (*after_close)(void *ctx);
    void *ctx;
};

static void _release_pair(_iopipe_pair_t *pair)
{
    zaio_free(pair->client, 1);
    zaio_free(pair->server, 1);
    if (pair->after_close) {
        pair->after_close(pair->ctx);
    }
    zfree(pair);
}

static void _after_read(zaio_t *aio);

static void _begin_read(zaio_t *aio)
{
    _iopipe_pair_t *pair = (_iopipe_pair_t *)zaio_get_context(aio);

    if (zaio_get_result(aio) < 1) {
        _release_pair(pair);
        return;
    }

    zaio_read(pair->client, _iopipe_buffer_size, _after_read);
    zaio_read(pair->server, _iopipe_buffer_size, _after_read);
}

static void _after_read(zaio_t *aio)
{
    _iopipe_pair_t *pair = (_iopipe_pair_t *)zaio_get_context(aio);

    int ret = zaio_get_result(aio);
    if ((ret < 1) || (ret > _iopipe_buffer_size)) {
        _release_pair(pair);
        return;
    }

    ZSTACK_BUF(bf, _iopipe_buffer_size+1);

    zaio_get_read_cache(aio, bf, ret);
    zaio_disable(aio);

    zaio_t *another_aio = (pair->client==aio?pair->server:pair->client);
    zaio_cache_write(another_aio, zbuf_data(bf), ret);
    zaio_cache_flush(another_aio, _begin_read);
}

void zaio_iopipe_enter(zaio_t *client, zaio_t *server, zaio_base_t *aiobase, void (*after_close)(void *ctx), void *ctx)
{
    _iopipe_pair_t *pair = (_iopipe_pair_t *)zcalloc(1, sizeof(_iopipe_pair_t));
    pair->client = client;
    pair->server = server;
    pair->after_close = after_close;
    pair->ctx = ctx;
    
    zaio_set_context(client, pair);
    zaio_set_context(server, pair);

    zaio_rebind_aio_base(client, aiobase);
    zaio_rebind_aio_base(server, aiobase);

    zaio_sleep(client, _begin_read, 0);
}

/* Local variables:
* End:
* vim600: fdm=marker
*/
