/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-10-15
 * ================================
 */

typedef struct _line_request_ctx_t _line_request_ctx_t;
struct _line_request_ctx_t {
    zaio_t *aio;
    zargv_t *cmdv;
    int argc;
    char **argv;
    char *cmdname;
    char *extra_data_ptr;
    int extra_data_len;
};

typedef struct _line_request_handler_t _line_request_handler_t;
struct _line_request_handler_t {
    const char *cmdname;
    void (*fn)(zaio_t *aio);
};

static int _line_request_extra_data_len_max = 10 * 1024 * 1024;
static _line_request_handler_t *_line_request_handler_vector = 0;

static void _do_after_response(zaio_t *aio);
static void _do_request_once(void *ctx);
static void _release_aio(zaio_t *aio)
{
    zaio_free(aio, 1);
}

static void _line_request_ctx_free(_line_request_ctx_t *rctx)
{
    if (!rctx) {
        return;
    }
    zargv_free(rctx->cmdv);
    zfree(rctx->extra_data_ptr);
    zfree(rctx);
}

static void _do_request_once(void *ctx)
{
    zaio_t *aio = (zaio_t *)ctx;
    _line_request_ctx_t *rctx = (_line_request_ctx_t *)zaio_get_context(aio);
    do { 
        if ((!rctx) || (rctx->argc < 1)) {
            _release_aio(aio);
            break;
        }
        const char *cmdname = rctx->cmdname;
        if ((!strcmp(cmdname, "exit")) || (!strcmp(cmdname, "quit"))) {
            zaio_cache_puts(aio, "OK BYE\n");
            zaio_cache_flush(aio, _release_aio);
            break;
        }

        int flag = 0;
        for (_line_request_handler_t *hd = _line_request_handler_vector; (hd && hd->cmdname); hd++) {
            if (!strcmp(cmdname, hd->cmdname)) {
                flag = 1;
                hd->fn(aio);
                break;
            }
        }
        if (flag == 0) {
            _release_aio(aio);
        }
    } while (0);

    _line_request_ctx_free(rctx);
}

static void _do_line_request_with_length_data(zaio_t *aio)
{
   _line_request_ctx_t *rctx = (_line_request_ctx_t *)zaio_get_context(aio);
    if (zaio_get_result(aio) < rctx->extra_data_len + 2) {
        _release_aio(aio);
       _line_request_ctx_free(rctx);
        return;
    }
    rctx->extra_data_ptr = (char *)zmalloc(rctx->extra_data_len + 2 + 1);
    zaio_get_read_cache_to_buf(aio, rctx->extra_data_ptr, rctx->extra_data_len + 2);
    rctx->extra_data_ptr[rctx->extra_data_len] = 0;
    zaio_disable(aio);
    zpthread_pool_job(worker_pool, _do_request_once, aio);
}

static void _do_line_request(zaio_t *aio)
{
    int ret = zaio_get_result(aio);
    if (ret < 1) {
        _release_aio(aio);
        return;
    }
    zbuf_t *req_line = zbuf_create(1024);
    zaio_get_read_cache(aio, req_line, ret);
    zargv_t *cmdv = zargv_create(6);
    if (strchr(zbuf_data(req_line), '|')) {
        zargv_split_append(cmdv, zbuf_data(req_line), "|\r\n");
    } else {
        zargv_split_append(cmdv, zbuf_data(req_line), " \r\n");
    }
    zbuf_free(req_line);
    if (zargv_len(cmdv) < 1) {
        zargv_free(cmdv);
        _release_aio(aio);
        return;
    }
   _line_request_ctx_t *rctx = (_line_request_ctx_t *)zcalloc(1, sizeof(_line_request_ctx_t));
    zaio_set_context(aio, rctx);
    rctx->aio = aio;
    rctx->cmdv = cmdv;
    rctx->argc = zargv_argc(cmdv);
    rctx->argv = zargv_argv(cmdv);
    rctx->cmdname = zblank_buffer;
    if (zargv_len(cmdv) > 0) {
        rctx->cmdname = rctx->argv[0];
    }
    rctx->extra_data_ptr = zblank_buffer;
    rctx->extra_data_len = 0;
    if ((zargv_data(cmdv)[zargv_len(cmdv) - 1])[0] == '{') {
        rctx->extra_data_len = atoi((zargv_data(cmdv)[zargv_len(cmdv) - 1]) + 1);
        if ((rctx->extra_data_len < 1)  || (rctx->extra_data_len > _line_request_extra_data_len_max)) {
            _release_aio(aio);
            return;
        }
        zaio_readn(aio, rctx->extra_data_len + 2, _do_line_request_with_length_data);
    } else {
        zaio_disable(aio);
        zpthread_pool_job(worker_pool, _do_request_once, aio);
    }
}

static void _do_after_response(zaio_t *aio)
{
    if (zaio_get_result(aio) < 1) {
        _release_aio(aio);
        return;
    }
    zaio_gets(aio, 4096, _do_line_request);
}

static void _do_service(int fd)
{
    znonblocking(fd, 1);
    zaio_t *aio = zaio_create(fd, zvar_default_aio_base);
    zaio_gets(aio, 4096, _do_line_request);
}

