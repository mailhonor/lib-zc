/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-11-20
 * ================================
 */

#define ZCC_AIO_NEED_PUBLIC
#include "../event/aio.cpp"

namespace zcc
{

aio_server *var_default_aio_server = 0;

aio_server::aio_server()
{
}

aio_server::~aio_server()
{
    if (var_default_aio_base) {
        delete var_default_aio_base;
        var_default_aio_base = 0;
    }
}

void aio_server::general_service_register(aio_base *ab, int fd, int fd_type, void (*callback) (int))
{
    zaio_base_t *b = zvar_default_aio_base;
    if (ab) {
        b = ((aio_base_public *)(ab))->get_aio_base();
    }
    zaio_server_general_aio_register(b, fd, fd_type, callback);
}

void aio_server::before_service(void)
{
}

void aio_server::before_softstop(void)
{
}

void aio_server::stop_notify(int stop_after_second)
{
    zaio_server_stop_notify(stop_after_second);
}

void aio_server::detach_from_master(void)
{
    zaio_server_detach_from_master();
}

static void _service_register(const char *service, int fd, int fd_type)
{
    var_default_aio_server->service_register(service, fd, fd_type);
}

static void _before_service(void)
{
    if (var_default_aio_base == 0) {
        var_default_aio_base = new aio_base(zvar_default_aio_base);
    }
    var_default_aio_server->before_service();
}

static void _before_softstop(void)
{
    var_default_aio_server->before_softstop();
}

static void _init_all(aio_server *as)
{
    var_default_aio_server = as;
    zaio_server_service_register = _service_register;
    zaio_server_before_service = _before_service;
    zaio_server_before_softstop = _before_softstop;
}

static void _fini_all()
{
}

int aio_server::run(int argc, char **argv)
{
    _init_all(this);
    int ret =  zaio_server_main(argc, argv);
    _fini_all();
    return ret;
}

} /* namespace zcc */

