# !/bin/bash

nohup /mnt/bcts_quote/frontserver/platform/build/front_server /mnt/bcts_quote/frontserver/platform/build/config.json  > /mnt/bcts_quote/frontserver/platform/build/front_server.log &

sleep 1s

ps -aux|grep front_server