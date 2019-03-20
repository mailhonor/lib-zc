/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-03-15
 * ================================
 */


#include "zc.h"
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sqlite3.h>

#ifdef _LIB_ZC_SQLITE3_

#define zpthread_lock(l)    {if(pthread_mutex_lock((pthread_mutex_t *)(l))){zfatal("mutex:%m");}}
#define zpthread_unlock(l)  {if(pthread_mutex_unlock((pthread_mutex_t *)(l))){zfatal("mutex:%m");}}

static void after_response(zaio_t *aio);
static char *sqlite3_proxy_filename = 0;
static int sqlite3_fd = 0;
static sqlite3 *sqlite3_handler = 0;
static pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t pth_proxy, pth_log;
/* {{{ proxy */
static zlist_t *proxy_list; /* <zaio_t *> */
static pthread_mutex_t proxy_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t proxy_cond = PTHREAD_COND_INITIALIZER;

static void proxy_exec(zaio_t *aio)
{
    int err = 0;
    char *err_sql = 0;
    zbuf_t *bf = (zbuf_t *)zaio_get_context(aio);
    char *sql = zbuf_data(bf) + 1;
    do {
        if (sqlite3_exec(sqlite3_handler, "BEGIN;", NULL, NULL, &err_sql) != SQLITE_OK) {
            err = 1;
            break;
        }
        if (sqlite3_exec(sqlite3_handler, sql, NULL, NULL, &err_sql) != SQLITE_OK) {
            err = 1;
            break;
        }
        if (sqlite3_exec(sqlite3_handler, "COMMIT;", NULL, NULL, &err_sql) != SQLITE_OK) {
            err = 1;
            break;
        }
    } while (0);
    if (err) {
        char *err_sql2 = 0;
        if (sqlite3_exec(sqlite3_handler, "ROLLBACK;", NULL, NULL, &err_sql2) != SQLITE_OK) {
            zfatal("FATAL sqlite3 rollback");
        }
    }

    zbuf_reset(bf);
    if (err) {
        zbuf_put(bf, 'E');
        zbuf_puts(bf, err_sql);
    } else {
        zbuf_put(bf, 'O');
    }
    zaio_cache_write_size_data(aio, zbuf_data(bf), zbuf_len(bf));
    zaio_cache_flush(aio, after_response, -1);
}

static void proxy_query(zaio_t *aio)
{
    int err = 0, status;
    sqlite3_stmt *sql_stmt = 0;
    int ncolumn, coi;
    zbuf_t *bf = (zbuf_t *)zaio_get_context(aio);
    char *sql = zbuf_data(bf) + 1;
    int len = zbuf_len(bf) - 1;
    do {
        if (sqlite3_prepare_v2(sqlite3_handler, sql, len, &sql_stmt, 0) != SQLITE_OK) {
            err = 1;
            break;
        }
        zbuf_reset(bf);
        ncolumn = sqlite3_column_count(sql_stmt);
        zbuf_printf_1024(bf, "O%d", ncolumn);
        zaio_cache_write_size_data(aio, zbuf_data(bf), zbuf_len(bf));

        zbuf_reset(bf);
        while ((status = sqlite3_step(sql_stmt)) != SQLITE_DONE) {
            if (status != SQLITE_ROW) {
                err = 1;
                break;
            }

            zbuf_reset(bf);
            zbuf_put(bf, '*');
            for (coi=0;coi<ncolumn;coi++) {
                char *d = (char *)sqlite3_column_blob(sql_stmt, coi);
                int l = sqlite3_column_bytes(sql_stmt, coi);
                zsize_data_escape(bf, d, l);
            }
            zaio_cache_write_size_data(aio, zbuf_data(bf), zbuf_len(bf));
        }
        if (err) {
            break;
        }
    } while(0);

    zbuf_reset(bf);
    zbuf_put(bf, (err?'E':'O'));
    if (err) {
        zbuf_puts(bf, sqlite3_errmsg(sqlite3_handler));
    }
    zaio_cache_write_size_data(aio, zbuf_data(bf), zbuf_len(bf));
    if (sql_stmt) {
        sqlite3_finalize(sql_stmt);
    }
    zaio_cache_flush(aio, after_response, -1);
}

static void *pthread_proxy(void *arg)
{
    pthread_detach(pthread_self());
    struct timespec timeout;
    zaio_t *aio;

    while(1) {
        if (zvar_proc_stop) {
            return 0;
        }
        zpthread_lock(&proxy_mutex);
        while(zbuf_len(proxy_list) == 0) {
            timeout.tv_sec = time(0) + 1;
            timeout.tv_nsec = 0;
            pthread_cond_timedwait(&proxy_cond, &proxy_mutex, &timeout);
            if (zvar_proc_stop) {
                zpthread_unlock(&proxy_mutex);
                return 0;
            }
        }
        zlist_shift(proxy_list, (void **)&aio);
        zpthread_unlock(&proxy_mutex);
        
        zbuf_t *bf = (zbuf_t *)zaio_get_context(aio);
        char *p = zbuf_data(bf);
        zpthread_lock(&global_mutex);
        if (p[0] == 'E') {
            proxy_exec(aio);
        } else {
            proxy_query(aio);
        }
        zpthread_unlock(&global_mutex);
        zbuf_free(bf);
    }

    return arg;
}
/* }}} */

/* {{{ log */
static zlist_t *log_list; /* <zbuf_t *> */
static zlist_t *log_list_tmp; /* <zbuf_t *> */
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t log_cond = PTHREAD_COND_INITIALIZER;

static void proxy_log()
{
    if (zvar_proc_stop) {
        return;
    }
    zpthread_lock(&global_mutex);
    if (zlist_len(log_list_tmp) == 0) {
        zpthread_unlock(&global_mutex);
        return;
    }

    char *err_sql = 0;
    int sok = 1;
    if (sok && (sqlite3_exec(sqlite3_handler, "BEGIN;", NULL, NULL, &err_sql) != SQLITE_OK)) {
        sok = 0;
    }
    ZLIST_WALK_BEGIN(log_list_tmp, zbuf_t *, bf) {
        char *sql = zbuf_data(bf) + 1;
        if (sok && (sqlite3_exec(sqlite3_handler, sql, NULL, NULL, &err_sql) != SQLITE_OK)) {
            sok = 0;
        }
        zbuf_free(bf);
    } ZLIST_WALK_END;
    zlist_reset(log_list_tmp);
    if (sok) {
        if (sok && (sqlite3_exec(sqlite3_handler, "COMMIT;", NULL, NULL, &err_sql) != SQLITE_OK)) {
            sok = 0;
        }
    }
    if (!sok) {
        if (sqlite3_exec(sqlite3_handler, "ROLLBACK;", NULL, NULL, &err_sql) != SQLITE_OK) {
            zfatal("FATAL sqlite3 rollback");
        }
    }

    zpthread_unlock(&global_mutex);
    if ((!sok) && err_sql) {
        zinfo("sqlite3 proxy log error: %s", err_sql);
    }
}

static void *pthread_log(void *arg)
{
    pthread_detach(pthread_self());
    struct timespec timeout;
    int cache_size = 0;

    while(1) {
        if (zvar_proc_stop) {
            return 0;
        }
        zpthread_lock(&log_mutex);
        long last_log = ztimeout_set(0);
        while(zlist_len(log_list) == 0) {
            timeout.tv_sec = time(0) + 1;
            timeout.tv_nsec = 0;
            pthread_cond_timedwait(&log_cond, &log_mutex, &timeout);
            if (zvar_proc_stop) {
                zpthread_unlock(&log_mutex);
                return 0;
            }
            if (ztimeout_set(0) - last_log > 1024) {
                last_log = ztimeout_set(0);
                proxy_log();
                cache_size = 0;
            }
        }
        zbuf_t *bf;
        zlist_shift(log_list, (void **)&bf);
        zpthread_unlock(&log_mutex);

        zlist_push(log_list_tmp, bf);
        int len = zbuf_len(bf) - 1;
        cache_size += len;
        if (cache_size > 1024 * 1024) {
            last_log = ztimeout_set(0);
            proxy_log();
            cache_size = 0;
        }
    }

    return arg;
}
/* }}} */

/* {{{ server */
static void release_aio(zaio_t *aio)
{
    zaio_free(aio, 1);
}

static void do_request(zaio_t *aio)
{
    int ret = zaio_get_result(aio);
    if (ret < 2) {
        release_aio(aio);
        return;
    }
    zbuf_t *bf = zbuf_create(ret);
    zaio_fetch_rbuf(aio, bf, ret);
    char *ptr = zbuf_data(bf);
    if ((ptr[0] =='E') || (ptr[0] == 'Q')) {
        zaio_set_context(aio, bf);
        zpthread_lock(&proxy_mutex);
        zlist_push(proxy_list, aio);
        zpthread_unlock(&proxy_mutex);
        pthread_cond_signal(&proxy_cond);
        return;
    } else if (ptr[0] == 'L') {
        zpthread_lock(&log_mutex);
        zlist_push(log_list, bf);
        zpthread_unlock(&log_mutex);
        pthread_cond_signal(&log_cond);
        zaio_read_size_data(aio, do_request, -1);
        return;
    } else {
        zbuf_free(bf);
        release_aio(aio);
        return;
    }
    return;
}

static void after_response(zaio_t *aio)
{
    if (zaio_get_result(aio) < 1) {
        release_aio(aio);
        return;
    }
    zaio_read_size_data(aio, do_request, -1);
}


static void do_slqite3_service(int fd)
{
    znonblocking(fd, 1);
    zaio_t *aio = zaio_create(fd, zvar_default_event_base);
    zaio_read_size_data(aio, do_request, 100);
}

void (*zsqlite3_proxy_server_before_service)() = 0;
void (*zsqlite3_proxy_server_before_reload)() = 0;
void (*zsqlite3_proxy_server_before_exit)() = 0;

static void ___service_register(const char *service_name, int fd, int fd_type)
{
    zevent_server_general_aio_register(zvar_default_event_base, fd, fd_type, do_slqite3_service);
}

static void ___before_service()
{
    do {
        proxy_list = zlist_create();
        log_list = zlist_create();
        log_list_tmp = zlist_create();
    } while(0);

    do {
        sqlite3_proxy_filename = zconfig_get_str(zvar_default_config, "sqlite3-proxy-filename", "");
        if(zempty(sqlite3_proxy_filename)) {
            zfatal("FATAL must set sqlite3-proxy-filename'value");
        }
        sqlite3_fd = open(sqlite3_proxy_filename, O_CREAT|O_RDWR, 0666);
        if (sqlite3_fd == -1) {
            zfatal("FATAL open %s(%m)", sqlite3_proxy_filename);
        }
        zflock_exclusive(sqlite3_fd);
        if (SQLITE_OK != sqlite3_open(sqlite3_proxy_filename, &sqlite3_handler)) {
            zfatal("FATAL dbopen %s(%m)", sqlite3_proxy_filename);
        }
        zinfo("sqlite3_proxy open %s", sqlite3_proxy_filename);
    } while(0);

    do {
        pthread_create(&pth_proxy, 0, pthread_proxy, 0);
        pthread_create(&pth_log, 0, pthread_log, 0);
    } while(0);
    
    if (zsqlite3_proxy_server_before_service) {
        zsqlite3_proxy_server_before_service();
    }
}

static void ___before_exit()
{
    zpthread_lock(&global_mutex);
    if (sqlite3_handler) {
        if (sqlite3_close(sqlite3_handler) != SQLITE_OK) {
            zfatal("FATAL close sqlite %s(%s)", sqlite3_proxy_filename, sqlite3_errmsg(sqlite3_handler));
        }
    }
    sqlite3_handler = 0;
    zlist_free(proxy_list);
    zlist_free(log_list);
    zlist_free(log_list_tmp);
    zpthread_unlock(&global_mutex);

    if (zsqlite3_proxy_server_before_exit) {
        zsqlite3_proxy_server_before_exit();
    }
}

int zsqlite3_proxy_server_main(int argc, char **argv)
{
    zevent_server_service_register = ___service_register;
    zevent_server_before_service = ___before_service;
    if (zsqlite3_proxy_server_before_reload) {
        zevent_server_before_reload = zsqlite3_proxy_server_before_reload;
    }
    zevent_server_before_exit = ___before_exit;
    return zevent_server_main(argc, argv);
}

/* }}} */

/* Local variables:
* End:
* vim600: fdm=marker
*/

#endif /* _LIB_ZC_SQLITE3_ */
