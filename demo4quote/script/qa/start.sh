# !/bin/bash

nohup /mnt/bcts_quote/demo4quote/cpp/build/demo4quote -qa  > /mnt/bcts_quote/demo4quote/cpp/build/quote.log  &

sleep 1s

ps -aux|grep demo4quote