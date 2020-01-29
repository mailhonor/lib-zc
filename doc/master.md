## master/server 进程服务管理框架
源码见 src/master/

---

```
/* 管理器 */

extern zbool_t zvar_master_server_log_debug_enable;
extern int zvar_master_server_reload_signal; /* SIGHUP */

/* master 重新加载各服务配置函数 */
/* zvector_t *cfs, 是 zconfig_t *的vector, 一个zconfig_t 对应一个服务 */
extern void (*zmaster_server_load_config)(zvector_t *cfs);

/* master进入服务管理前执行的函数 */
extern void (*zmaster_server_before_service)();

/* 一个通用的加载一个目录下所有服务配置的函数 */
void zmaster_server_load_config_from_dirname(const char *config_dir_pathname, zvector_t *cfs);

/* master程序主函数 */
int zmaster_server_main(int argc, char **argv);

```

---


```
/* 基于事件(zaio_base_t 模型)的服务模型  */
/* 主线程运行在 zaio_base_t 框架下, event_hase 为 zvar_default_aio_base */

/* 注册服务, service 是服务名, fd继承自master, fd_type: inet/unix/fifo 见(zvar_tcp_listen_type_inter ...) */
extern void (*zevent_server_service_register) (const char *service, int fd, int fd_type);

/* 进入主服务前执行函数 */
extern void (*zevent_server_before_service) (void);

/* 接到master重启之前后执行的函数 */
extern void (*zevent_server_before_reload) (void);

/* 退出前执行函数 */
extern void (*zevent_server_before_exit) (void);

/* 手动通知主程序循环退出 */
void zevent_server_stop_notify(void);

/* 通用的服务注册函数 */
/* fd2 = accept(fd); callback(fd2) */
zeio_t *zevent_server_general_aio_register(zaio_base_t *eb, int fd, int fd_type, void (*callback) (int));

/* 主函数 */
int zevent_server_main(int argc, char **argv);
```

---

```
/* 基于协程(zcoroutine_base_init)的服务模型  */
/* 主线程运行在协程框架下 */

/* 注册服务, service 是服务名, fd继承自master, fd_type: inet/unix/fifo 见(zvar_tcp_listen_type_inter ...) */
extern void (*zcoroutine_server_service_register) (const char *service, int fd, int fd_type);

/* 进入主服务前执行函数 */
extern void (*zcoroutine_server_before_service) (void);

/* 接到master重启之前后执行的函数 */
extern void (*zcoroutine_server_before_reload) (void);

/* 手动通知主程序循环退出 */
extern void (*zcoroutine_server_before_exit) (void);

/* 手动通知主程序循环退出 */
void zcoroutine_server_stop_notify(void);

/* 主函数 */
int zcoroutine_server_main(int argc, char **argv);

```

---

### 例子
见源码 sample/master/
