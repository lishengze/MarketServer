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
1. symbol 元信息  
   通过websocket 服务获取.  
   1) 初始化： 当websocket 建立时， 服务端会将当前存储的 symbol 发送给客户端. 
        json: {"type": "symbol_list", "symbol": ["symbol1", "symbol2" ]}
   2) 更新:  
       当有新的合约加入时，websocket 会推送更新json 过来
       json: {"type": "symbol_update", "symbol": ["symbol1", "symbol2" ]}

2. depth数据 
    通过websocket服务获取与更新
    1) 发送订阅请求，请求的json字符串为:  
    {  
        "type":"sub_symbol",  
        "symbol":["symbolName"]  //需要订阅的symbole数组   
    }  

    2) websocket 推送的数据:
    {  
        "ask_length":numb,  
        "asks":[[price, volume, accumulatedVolume]...],  // 每个数组原子按顺序存储 价格，成交量，累积成交量等信息;  
        "bid_length":numb,  
        "bids":[[price, volume, accumulatedVolume]...],  
        "symbol":"symbolName",  
        "exchange":"",  
        "seqno":0,  // 当前行情序列号  
        "tick":0,   // 时间戳  
        "type":"market_data_update"     // type 类型   

    }

3. K线数据
    通过websocket服务获取与更新
    1) 发送订阅请求，请求的json字符串为:     
    {   
        "type":"kline_update",    
        "symbol":"symbolName",  
        "start_time":start_time,    // 必须是秒级的UTC时间戳   
        "end_time":end_time,        // 必须是秒级的UTC时间戳  
        "data_count": data_count,   // 请求的数据数目，这个选项和起止时间只需填写一个
        "frequency":"60"            // 数据频率，以秒为单位，现在必须是60的整数倍.
    }

    2) websocket 推送的数据:
    {  
        "data":[["open":,"high":,"low":,"close":,"volume":,"tick":,]...]  // 每个数组原子储存 open, high, low, close, volume, tick-时间戳 等信息;
        "symbol":"symbolName",    
        "start_time":"",    // 回复数据的开始时间，秒级的UTC时间戳   
        "end_time":0,       // 回复数据的结束时间，秒级的UTC时间戳   
        "data_count":0,     // 回复的数据数目
        "frequency":0,      // 请求的时间频率  
        "type":"kline_update"     // type 类型   


    }    