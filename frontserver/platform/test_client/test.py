import websocket
import json
import urllib.request
import http.client
import time
import _thread

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


def sub_btc_usdt(ws, sub_symbol):
    time.sleep(10)

    sub_info = {
        "type":"sub_symbol",
        "symbol":[sub_symbol]
    }
    sub_info_str = json.dumps(sub_info)
    print("sub_info_str: %s" % (sub_info_str))
    ws.send(sub_info_str)    

def on_open(ws):
    print("Connected")
    sub_info = {
        "type":"sub_symbol",
        "symbol":["XRP_USDT"]
    }
    sub_info_str = json.dumps(sub_info)
    print("sub_info_str: %s" % (sub_info_str))
    ws.send(sub_info_str)

    _thread.start_new_thread( sub_btc_usdt, (ws, "BTC_USDT", ) )


def test_websocket():
    # websocket.enableTrace(True)
    # ip = "ws://36.255.220.139"
    ip = "ws://127.0.0.1"
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

def test_http_client():
    uri = "http://127.0.0.1:9115"
    conn = http.client.HTTPConnection(uri)  
    conn.request("GET", "/v1/test_client")
    r1 =conn.getresponse()
    print(r1)  

def test_urllib():
    uri = "http://127.0.0.1:9115"
    response = urllib.request.urlopen(uri)
    html = response.read()
    print("Http Response: \n%s" % (html))    

    
if __name__ == "__main__":
    test_websocket()

    # test_http_restful()

