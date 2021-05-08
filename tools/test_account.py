from concurrent import futures
import time
import math
import logging

import grpc

import risk_controller_pb2
import risk_controller_pb2_grpc

def CallOtcQuery(addr):
    with grpc.insecure_channel(addr) as channel:
        stub = risk_controller_pb2_grpc.RiskControllerStub(channel)
        request = risk_controller_pb2.QuoteRequest()
        request.symbol = "BTC_USDT"
        request.direction = 0
        request.amount = 0.025
        request.turnover = 0
        resp = stub.OtcQuote(request)
        return resp


def test_otc():
    address = "127.0.0.1:9111"
    ret = CallOtcQuery(address)
    print(ret)

if __name__ == '__main__':
    app = QApplication(sys.argv)
    ex = DisplayWidget()

    sys.exit(app.exec_())        