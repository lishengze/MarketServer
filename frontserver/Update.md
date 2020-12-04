# 升级日志  

## 2020.12.04

### 概述

改变websocket接口格式，增加心跳机制

1. 改变接口格式  
    每个请求json增加type字段;  
    当前请求订阅字段 增加的type 属性为: sub_symbol   
    例: {"type": "sub_symbol", "symbol": ["XRP_USDT"]}

2. 增加心跳机制  
   服务端每隔一段时间-当前是5秒，会发送心跳字段给客户端形式为：{"time":"2020-12-04 07:41:20.15205969","type":"heartbeat"}.  
   客户端需要回应  {"type": "heartbeat"} 这样的字符串即可.
   
    
