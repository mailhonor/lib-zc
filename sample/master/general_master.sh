#!/bin/sh

___INFO()
{
	echo $@
}
INFO=___INFO

subcmd=$1

work_path="$WORK_PATH"; [ -z $work_path ] && work_path="./"
master_cmd="$MASTER_CMD"; [ -z $master_cmd ] && master_cmd="./bin/master"
service_dir="$SERVICE_DIR"; [ -z $service_dir ] && service_dir="./etc/service"
pid_file="$PID_FILE"; [ -z $pid_file ] && pid_file="./var/pid/master.pid"

# master程序可提供日志服务:  masger 参数 -log-service
# log_service="./log.socket,./log_dir/,day"
# ./log.socket       日志服务的监听路径(domain socket)
# ./log_dir/         日志服务的日志保存目录
# day                日志文件分割, 一天一个文件. 另外可选hour

# master程序自己的日志输出:  master 参数 -server-log
# server_log="masterlog,./log.socket"        # 或
# server_log="syslog,mail,cmd_display_name"  # 或
# server_log="syslog,mail"

log_service="$LOG_SERVICE"
server_log="$SERVER_LOG"; [ -z $server_log ] && server_log="syslog,mail"

cd $work_path || {
	$INFO no such work_path \"$work_path\" !
	exit 1
}

[ -f "$ENV_SOURCE" ] && . $ENV_SOURCE ${work_path}

umask 002
touch $pid_file

case $subcmd in
	start)
		$master_cmd -pid-file $pid_file --try-lock 2>/dev/null || {
			$INFO the system is already running
			exit 1
		}
		cmdrun="$master_cmd -C $service_dir -pid-file $pid_file"
		[ "$log_service" = "" ] || {
			cmdrun=" $cmdrun -log-service $log_service"
		}
		[ "$server_log" = "" ] || {
			cmdrun=" $cmdrun -server-log $server_log"
		}
		$cmdrun &
		$master_cmd -sleep 100
		$master_cmd -pid-file $pid_file --try-lock 2>/dev/null && {
			$master_cmd -sleep 1000
			$master_cmd -pid-file $pid_file --try-lock 2>/dev/null && {
				$INFO "starting the system ERROR"
				exit 1
			}
		}
		$INFO starting the system OK
		;;

	stop)
		$master_cmd -pid-file $pid_file --try-lock 2>/dev/null && {
			$INFO  the system is not running
			exit 1
		}
		$INFO stop the system
		kill `head -n 1 $pid_file`
		for i in 5 4 3 2 1
		do
			$master_cmd -pid-file $pid_file --try-lock && exit 0
			$INFO waiting "for" the system to terminate
			$master_cmd -sleep 1000
		done
		;;

	reload)
		$master_cmd -pid-file $pid_file --try-lock 2>/dev/null && {
			$INFO  the system is not running
			exit 1
		}
		$INFO refreshing the system
		kill -HUP `head -n 1 $pid_file`
		;;

	restart)
        sh $0 "stop"
        sh $0 start
        ;;
	*)
		$INFO unknown command: $subcmd
		$INFO USAGE: $0 start "("or stop, reload, restart")"
		;;

esac

