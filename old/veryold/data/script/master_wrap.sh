#!/bin/sh

___INFO()
{
	echo $@
}
INFO=___INFO

cmd=$1
work_path="./"
master_cmd="test/master"
config_file="data/master.cf"

cd $work_path || {
	$INFO no such work_path \"$work_path\" !
	exit 1
}
ulimit -n 102400
umask 002
mkdir -p zfile/pid
touch zfile/pid/master.pid

case $cmd in
	start)
		$master_cmd -test_lock 2>/dev/null || {
			$INFO the system is already running
			exit 1
		}
		$INFO starting the system
		$master_cmd $config_file &
		;;

	stop)
		$master_cmd -test_lock 2>/dev/null && {
			$INFO  the system is not running
			exit 1
		}
		$INFO stop the system
		kill `head -n 1 zfile/pid/master.pid`
		for i in 5 4 3 2 1
		do
			$master_cmd -test_lock && exit 0
			$INFO waiting "for" the system to terminate
			sleep 1
		done
		;;

	reload)
		$master_cmd -test_lock 2>/dev/null && {
			$INFO  the system is not running
			exit 1
		}
		$INFO refreshing the system
		kill -HUP `head -n 1 zfile/pid/master.pid`
		;;

	*)
		$INFO unknown command: $cmd
		$INFO USAGE: $0 start "("or stop, reload, restart")"
		;;

esac
