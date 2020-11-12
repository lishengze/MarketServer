from concurrent import futures
import time
import math
import logging

import grpc

import risk_controller_pb2
import risk_controller_pb2_grpc


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


def run():
    # NOTE(gRPC Python Team): .close() is possible on a channel and should be
    # used in circumstances in which the with statement does not fit the needs
    # of the code.    
    with grpc.insecure_channel('localhost:9900') as channel:
        stub = risk_controller_pb2_grpc.s(channel)
        responses = stub.ServeMarketStream(empty_pb2.Empty())
        for resp in responses:
            for quote in resp.quotes:
                display(quote)

if __name__ == '__main__':
    logging.basicConfig()
    #serve()
    run()
                            
