/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-11-10
 * ================================
 */

#define zvar_master_server_status_fd  3

#define zvar_event_server_status_fd   4
#define zvar_event_server_listen_fd   5

#define zvar_coroutine_server_status_fd   4
#define zvar_coroutine_server_listen_fd   5

void zmaster_load_global_config_from_dir_inner(zconfig_t *cf, const char *config_path);

void zmaster_log_use_inner(char *progname, char *log_info);
