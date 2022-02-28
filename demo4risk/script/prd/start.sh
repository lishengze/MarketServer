# !/bin/bash

nohup /mnt/bcts_quote/demo4risk/cpp/build/demo4risk -prd > /mnt/bcts_quote/demo4risk/cpp/build/risk.log  &

sleep 1s

ps -aux|grep demo4risk