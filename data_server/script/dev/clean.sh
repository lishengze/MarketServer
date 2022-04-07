# !/bin/bash

PID=`ps aux|grep data_server |grep -v grep | awk '{print $2}'`
kill -9 $PID

rm -fr data_server_log/

sleep 2s

ps -aux|grep data_server