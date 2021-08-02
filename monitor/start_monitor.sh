# !/bin/bash
nohup python3 monitor.py &

sleep 2s

ps -aux|grep monitor
