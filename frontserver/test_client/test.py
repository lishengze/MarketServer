import websocket
import json
import urllib.request
import http.client
import time
import _thread
import time

def process_heartbeat(ws):
    heartbeat_info = {
        "type":"heartbeat"
    }
    heartbeat_info_str = json.dumps(heartbeat_info)

    print("heartbeat_info_str: %s" % (heartbeat_info_str))
    ws.send(heartbeat_info_str)    

def on_message(ws, message):
    print("New Message")
    print(message)

    dic = json.loads(message)
    if dic["type"] == "heartbeat":
        process_heartbeat(ws)
    elif dic["type"] == "symbol_update":
        pass
    
def on_error(ws, error):
    print("Error")
    print(error)

def on_close(ws):
    print("Server Closed")
    print("### closed ###")

def get_sub_depth_str(symbol="BTC_USDT"):
    sub_info = {
        "type":"sub_symbol",
        "symbol":[symbol]
    }
    sub_info_str = json.dumps(sub_info)
    print("sub_info_str: %s" % (sub_info_str))
    return sub_info_str

def get_sub_trade_str(symbol="BTC_USDT"):
    sub_info = {
        "type":"trade",
        "symbol":[symbol]
    }
    sub_info_str = json.dumps(sub_info)
    print("sub_info_str: %s" % (sub_info_str))
    return sub_info_str    

def get_sub_kline_str(symbl="BTC_USDT"):
    print("get_sub_kline_str")
    frequency = 60
    end_time = int(time.time())
    end_time = end_time - end_time % frequency - frequency
    start_time = end_time - 60 * 30

    # sub_info = {
    #     "type":"kline_update",
    #     "symbol":"BTC_USDT",
    #     "start_time":str(start_time),
    #     "end_time":str(end_time),
    #     "frequency":str(frequency)
    # }

    sub_info = {
        "type":"kline_update",
        "symbol":symbl,
        "data_count":str(1440),
        "frequency":str(frequency)
    }

    sub_info_str = json.dumps(sub_info)
    print("\n\n\n****************************** sub_info_str: %s ****************************" % (sub_info_str))
    return sub_info_str

def sub_btc_usdt(ws, sub_symbol):
    time.sleep(5)

    # sub_info = {
    #     "type":"sub_symbol",
    #     "symbol":[sub_symbol]
    # }
    # sub_info_str = json.dumps(sub_info)
    # print("sub_info_str: %s" % (sub_info_str))

    sub_info_str = get_sub_kline_str(sub_symbol)

    # sub_info_str = get_sub_depth_str(sub_symbol)

    # sub_info_str = get_sub_trade_str(sub_symbol)

    print("\n\n\n****************************** sub_info_str: %s ****************************" % (sub_info_str))

    ws.send(sub_info_str)   

def on_open(ws):
    print("Connected")

    # send_str = get_sub_depth_str()

    send_str = get_sub_kline_str()

    # send_str = get_sub_trade_str()

    ws.send(send_str)

    # time.sleep(3)

    # send_str = get_sub_kline_str()

    # ws.send(send_str)


    # _thread.start_new_thread(sub_btc_usdt, (ws, "XRP_USDT", ) )

def test_websocket():
    # websocket.enableTrace(True)
    ip = "ws://36.255.220.139"
    # ip = "ws://127.0.0.1"
    port = 9114
    url = ip + ":" + str(port)
    print("\n\n***** Connect %s *****" % (url))
    
    ws = websocket.WebSocketApp(url,
                                on_message=on_message,
                                on_error=on_error,
                                on_close=on_close)
    ws.on_open = on_open
    ws.run_forever()

def test_http_restful():
    # test_urllib()
    test_http_client()

def get_http_kline_str():
    symbol = "BTC_USDT"
    frequency = 60 * 5
    end_time = int(time.time())
    end_time = end_time - end_time % frequency
    start_time = end_time - 60 * 30
    query_str = ("v1/kline_request/symbol=%s&start_time=%d&end_time=%d&frequency=%d" \
                % (symbol, start_time, end_time, frequency))
    return query_str  

def get_http_enquiry_str():
    symbol = "BTC_USDT"
    volume = 10
    direction_type = 0
    query_str = ("/v1/enquiry?symbol=%s&type=%d&volume=%d" \
                % (symbol, direction_type, volume))
    return query_str      

def test_http_client():
    # uri = "127.0.0.1:9115"
    uri = "36.255.220.139:9115"
    conn = http.client.HTTPConnection(uri)
    # query_str = get_http_kline_str()
    query_str = get_http_enquiry_str()

    print("query_str: %s" % (query_str))

    conn.request("GET", query_str)
    res =conn.getresponse()
    print(res.read().decode("utf-8"))
    
def test_urllib():
    uri = "http://127.0.0.1:9115"
    response = urllib.request.urlopen(uri)
    html = response.read()
    print("Http Response: \n%s" % (html))    

if __name__ == "__main__":
    test_websocket()
    # test_http_restful()

