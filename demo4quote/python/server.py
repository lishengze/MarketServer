from concurrent import futures
import time
import math
import logging

import grpc

import api_pb2
import api_pb2_grpc

class TradeServicer(api_pb2_grpc.TradeServicer):
    """Provides methods that implement functionality of route guide server."""

    def __init__(self):
        pass

    def PutMarketStream(self, request_iterator, context):
        for data in request_iterator:
            if data.symbol != "BTC_USDT":
                continue            
            print("{0}({2}) - {1}({3})".format(data.ask_depth[0].price, data.bid_depth[0].price, data.ask_depth[0].data[0].exchange, data.bid_depth[0].data[0].exchange))

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
                            
