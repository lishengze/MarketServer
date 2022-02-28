# !/bin/bash

PID=`ps aux|grep front_server |grep -v grep | awk '{print $2}'`
kill -9 $PID

rm -fr front_server_log/

sleep 2s

ps -aux|grep front_server