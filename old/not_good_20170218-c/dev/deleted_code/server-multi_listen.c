/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-20
 * ================================
 */


//#include "libzc.h"
//#include <signal.h>
//
//zmaster_server_service_t *zmaster_server_service_list = 0;
//zmaster_server_cb_t zmaster_server_before_service = 0;
//zmaster_server_cb_t zmaster_server_on_reload = 0;
//zmaster_server_cb_t zmaster_server_on_loop = 0;
//
//static int test_mode;
//static int stopped;
//static zev_t *ev_status;
//
//static int after_master_reload(zev_t * zev)
//{
//    zev_fini(ev_status);
//    close(ZMASTER_STATUS_FD);
//
//    if (zmaster_server_on_reload)
//    {
//        zmaster_server_on_reload();
//    }
//    else
//    {
//        exit(0);
//    }
//
//    return 0;
//}
//
//static zmaster_server_service_t *find_service(char *id)
//{
//    zmaster_server_service_t *service;
//
//    for (service = zmaster_server_service_list; service->service_id; service++)
//    {
//        if (!strcmp(service->service_id, id))
//        {
//            return service;
//        }
//    }
//
//    return zmaster_server_service_list;
//}
//
//static int inet_server_accept(zev_t * zev)
//{
//    int fd;
//    int listen_fd;
//    zmaster_server_service_t *service;
//
//    listen_fd = zev_get_fd(zev);
//    service = (zmaster_server_service_t *) (zev_get_context(zev));
//
//    fd = zinet_accept(listen_fd);
//
//    if (fd < 0)
//    {
//        if (errno != EAGAIN)
//        {
//            zfatal("inet_server_accept: %m");
//        }
//        return -1;
//    }
//    znonblocking(fd, 1);
//
//    service->callback(fd, ZMASTER_LISTEN_INET);
//
//    return 0;
//}
//
//static int unix_server_accept(zev_t * zev)
//{
//    int fd;
//    int listen_fd;
//    zmaster_server_service_t *service;
//
//    listen_fd = zev_get_fd(zev);
//    service = (zmaster_server_service_t *) (zev_get_context(zev));
//
//    fd = zunix_accept(listen_fd);
//
//    if (fd < 0)
//    {
//        if (errno != EAGAIN)
//        {
//            zfatal("unix_server_accept: %m");
//        }
//        return -1;
//    }
//    znonblocking(fd, 1);
//
//    service->callback(fd, ZMASTER_LISTEN_UNIX);
//
//    return 0;
//}
//
//static void server_register(char *service_str)
//{
//    zmaster_server_service_t *service;
//    int type, sock_fd;
//    zev_t *zev;
//    char _service_str[1024];
//    char *stype, *uri, *p;
//
//    strcpy(_service_str, service_str);
//    stype = _service_str;
//    p = strstr(_service_str, "://");
//    if (p)
//    {
//        *p = 0;
//        uri = p + 3;
//    }
//    else
//    {
//        stype = "";
//        uri = _service_str;
//    }
//
//    if (!test_mode)
//    {
//        p = strchr(uri, ':');
//        if (p == 0)
//        {
//            zfatal("%s: args error: %s", zvar_progname, service_str);
//        }
//        *p = 0;
//        type = *uri;
//        sock_fd = atoi(p + 1);
//    }
//    else
//    {
//        int port;
//        p = strchr(uri, ':');
//        if (p)
//        {
//            type = ZMASTER_LISTEN_INET;
//            *p = 0;
//            p++;
//            port = atoi(p);
//            sock_fd = zinet_listen(uri, port, -1);
//            p[-1] = ':';
//        }
//        else
//        {
//            type = ZMASTER_LISTEN_UNIX;
//            sock_fd = zunix_listen(uri, -1);
//        }
//
//        if (sock_fd < 0)
//        {
//            zfatal("%s: open: %s error (%m)", zvar_progname, service_str);
//        }
//    }
//
//    service = find_service(stype);
//
//    if (service->raw_flag)
//    {
//        if (service->callback)
//        {
//            service->callback(sock_fd, type);
//        }
//        return;
//    }
//
//    zclose_on_exec(sock_fd, 1);
//    znonblocking(sock_fd, 1);
//    zev = (zev_t *) zmalloc(sizeof(zev_t));
//    zev_init(zev, zvar_evbase, sock_fd);
//    zev_set_context(zev, service);
//
//    if (type == ZMASTER_LISTEN_INET)
//    {
//        zev_set(zev, ZEV_READ, inet_server_accept);
//    }
//    else
//    {
//        zev_set(zev, ZEV_READ, unix_server_accept);
//    }
//}
//
//int zmaster_server_main(int argc, char **argv)
//{
//    int op;
//
//    if (!zvar_progname)
//    {
//        zvar_progname = argv[0];
//    }
//
//    signal(SIGPIPE, SIG_IGN);
//
//    if ((!zmaster_server_service_list) || (!zmaster_server_service_list->service_id))
//    {
//        zfatal("zmaster_server_service_list nothing");
//    }
//
//    if ((argc < 3) || (strcmp(argv[1], "-M")))
//    {
//        printf("parameters error");
//        exit(1);
//    }
//
//    if ((strcmp(argv[2], "master")) && strcmp(argv[2], "server"))
//    {
//        printf("parameters error");
//        exit(1);
//    }
//
//    test_mode = 1;
//    if ((!strcmp(argv[2], "master")))
//    {
//        test_mode = 0;
//    }
//
//    zvar_config_init();
//    zvar_evbase = zevbase_create();
//
//    while ((op = getopt(argc, argv, "M:l:c:o:dv")) > 0)
//    {
//        switch (op)
//        {
//        case 'M':
//            break;
//        case 'c':
//            zconfig_load(zvar_config, optarg);
//            break;
//        case 'l':
//            server_register(optarg);
//            break;
//        case 'o':
//            {
//                char *key, *value;
//                key = zstrdup(optarg);
//                value = strchr(key, '=');
//                if (value)
//                {
//                    *value++ = 0;
//                }
//                zconfig_add(zvar_config, key, value);
//                zfree(key);
//            }
//            break;
//        case 'd':
//            zlog_set_level_from_console(ZLOG_DEBUG);
//            break;
//        case 'v':
//            zlog_set_level_from_console(ZLOG_VERBOSE);
//            break;
//        default:
//            zfatal("args error");
//        }
//    }
//
//    if (zmaster_server_before_service)
//    {
//        zmaster_server_before_service();
//    }
//
//    zev_t ev_status_buf;
//    ev_status = &ev_status_buf;
//    zclose_on_exec(ZMASTER_STATUS_FD, 1);
//    zev_init(ev_status, zvar_evbase, ZMASTER_STATUS_FD);
//    if (!test_mode)
//    {
//        znonblocking(ZMASTER_STATUS_FD, 1);
//        zev_set(ev_status, ZEV_READ, after_master_reload);
//    }
//
//    stopped = 0;
//    while (1)
//    {
//        zevbase_dispatch(zvar_evbase, 0);
//        if (zmaster_server_on_loop)
//        {
//            zmaster_server_on_loop();
//        }
//        if (stopped)
//        {
//            break;
//        }
//    }
//
//    /*  release */
//    if (0)
//    {
//        /* */
//    }
//
//    return 0;
//}
//
//void zmaster_server_disconnect(int fd)
//{
//    close(fd);
//}
//
//void zmaster_server_stop_notify(void)
//{
//    stopped = 1;
//    zevbase_notify(zvar_evbase);
//}
//
