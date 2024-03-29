import websocket


def on_message(ws, message):
    print("New Message")
    print(message)


def on_error(ws, error):
    print("Error")
    print(error)


def on_close(ws):
    print("Server Closed")
    print("### closed ###")

def on_open(ws):
    print("Connected")
    ws.send("This is Python!")


def test():
    websocket.enableTrace(True)
    ip = "ws://localhost"
    port = 9114
    url = ip + ":" + str(port)
    print("\n\n***** Connect %s *****" % (url))
    
    ws = websocket.WebSocketApp(url,
                                on_message=on_message,
                                on_error=on_error,
                                on_close=on_close)
    ws.on_open = on_open
    ws.run_forever()
    
if __name__ == "__main__":
    test()

