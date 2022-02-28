# !/bin/bash

nohup /mnt/bcts_quote/frontserver/platform/build/front_server -qa > /mnt/bcts_quote/frontserver/platform/build/front_server.log &

sleep 1s

ps -aux|grep front_server