# 行情前置服务器

## 接口说明

### 服务类型

1. websoket 服务  
   a. websoket 的请求与推送都是以 json 字符串的形式完成. 

   b. 请求与推送的 json 中有固定的 type 属性说明请求与推送的数据类型.

   c. 心跳机制:  
    服务器会定时发送心跳数据包给客户端，type:"heartbeat", 客户端只需回复 {"type":"heartbeat"} 即可保持连接.

2. http 服务  
   a. http 的请求通过制定 方法与url完成, 请求的具体参数集合在 url 中.

   b. url 模板: versionName/requestName/param1=value1&param2=value2&param3=value3...

   c. 回调的数据是 json 形式的字符串.其中的 type 字段说明了回调的数据类型.

### 数据类型
1. symbol 列表  
   通过websocket 服务获取. 
   
2. depth数据 
    通过websocket服务


3. K线数据

### 心跳

## 升级日志  

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
   
    
