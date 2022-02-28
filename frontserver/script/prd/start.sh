# !/bin/bash

nohup /mnt/bcts_quote/frontserver/platform/build/front_server -prd > /mnt/bcts_quote/frontserver/platform/build/front_server.log &

sleep 1s

ps -aux|grep front_server