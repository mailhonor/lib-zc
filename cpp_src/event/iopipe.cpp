/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-11-08
 * ================================
 */

#ifdef __linux__
#include "zcc/zcc_aio.h"
#include "zcc/zcc_openssl.h"

zcc_namespace_begin;

#define _iopipe_buffer_size 4096

struct _iopipe_pair_t
{
    aio *client{nullptr};
    aio *server{nullptr};
    std::function<void()> after_close;
};

static void _release_pair(_iopipe_pair_t *pair)
{
    if (pair->client)
    {
        delete pair->client;
    }
    if (pair->server)
    {
        delete pair->server;
    }
    pair->after_close();
    delete pair;
}

static void _after_read(aio *ev, _iopipe_pair_t *pair);

static void _begin_read(aio *ev, _iopipe_pair_t *pair)
{
    if (ev->get_result() < 1)
    {
        _release_pair(pair);
        return;
    }

    pair->client->read(_iopipe_buffer_size, std::bind(_after_read, pair->client, pair));
    pair->server->read(_iopipe_buffer_size, std::bind(_after_read, pair->server, pair));
}

static void _after_read(aio *ev, _iopipe_pair_t *pair)
{
    int ret = ev->get_result();
    if ((ret < 1) || (ret > _iopipe_buffer_size))
    {
        _release_pair(pair);
        return;
    }
    char buf[_iopipe_buffer_size + 1];
    ev->get_read_cache(buf, ret);
    ev->disable();

    aio *another_aio = (pair->client == ev ? pair->server : pair->client);
    another_aio->cache_write(buf, ret);
    another_aio->cache_flush(std::bind(_begin_read, another_aio, pair));
}

void aio_iopipe_enter(aio *client, aio *server, aio_base *aiobase, std::function<void()> after_close)
{
    _iopipe_pair_t *pair = new _iopipe_pair_t();
    pair->client = client;
    pair->server = server;
    pair->after_close = after_close;

    client->rebind_aio_base(aiobase);
    server->rebind_aio_base(aiobase);

    client->sleep(std::bind(_begin_read, client, pair), 0);
}

zcc_namespace_end;

#endif // __linux__