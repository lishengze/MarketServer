# !/bin/bash

PID=`ps aux|grep demo4quote |grep -v grep | awk '{print $2}'` 
kill -9 $PID

sleep 2s

ps -aux|grep demo4quote

