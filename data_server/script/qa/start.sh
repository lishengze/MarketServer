# !/bin/bash

nohup /mnt/bcts_quote/data_server/cpp/build/data_server -qa > /mnt/bcts_quote/data_server/cpp/build/risk.log  &

sleep 1s

ps -aux|grep data_server