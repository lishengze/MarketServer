from concurrent import futures
import time
import math
import logging

import grpc

import api_pb2
import api_pb2_grpc


def convert_depth(depths):
    if len(depths) == 0:
        return "-"
    price = depths[0].price
    exchanges = [item.exchange for item in depths[0].data]
    return "{0}({1})".format(price, ",".join(exchanges))

def display_depth(prefix, depth):
    exchanges = [item.exchange for item in depth.data]
    print("{2}:{0}({1})".format(depth.price, ",".join(exchanges), prefix))

def display(data):
    print("{0} / {1}".format(data.symbol, data.msg_seq))
    for depth in data.ask_depth[::-1]:
        display_depth("ask", depth)
    for depth in data.bid_depth:
        display_depth("bid", depth)
    print("-----------------------------------")

class TradeServicer(api_pb2_grpc.TradeServicer):
    """Provides methods that implement functionality of route guide server."""

    def __init__(self):
        pass

    def PutMarketStream(self, request_iterator, context):
        for data in request_iterator:
            if data.symbol != "BTC_USDT":
                continue
            display(data)

def serve():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    api_pb2_grpc.add_TradeServicer_to_server(
        TradeServicer(), server)
    server.add_insecure_port('[::]:9000')
    server.start()
    server.wait_for_termination()


if __name__ == '__main__':
    logging.basicConfig()
    serve()
                            
