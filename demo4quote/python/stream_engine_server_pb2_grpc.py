# Generated by the gRPC Python protocol compiler plugin. DO NOT EDIT!
"""Client and server classes corresponding to protobuf-defined services."""
import grpc

import stream_engine_server_pb2 as stream__engine__server__pb2


class StreamEngineServiceStub(object):
    """Trade grpc
    """

    def __init__(self, channel):
        """Constructor.

        Args:
            channel: A grpc.Channel.
        """
        self.GetQuote = channel.unary_unary(
                '/trade.service.v1.StreamEngineService/GetQuote',
                request_serializer=stream__engine__server__pb2.GetQuoteReq.SerializeToString,
                response_deserializer=stream__engine__server__pb2.QuoteData.FromString,
                )
        self.SubscribeOneQuote = channel.unary_stream(
                '/trade.service.v1.StreamEngineService/SubscribeOneQuote',
                request_serializer=stream__engine__server__pb2.GetQuoteReq.SerializeToString,
                response_deserializer=stream__engine__server__pb2.MultiQuoteData.FromString,
                )
        self.MultiSubscribeQuote = channel.unary_stream(
                '/trade.service.v1.StreamEngineService/MultiSubscribeQuote',
                request_serializer=stream__engine__server__pb2.SubscribeQuoteReq.SerializeToString,
                response_deserializer=stream__engine__server__pb2.MultiQuoteData.FromString,
                )
        self.MultiSubscribeHedgeQuote = channel.unary_stream(
                '/trade.service.v1.StreamEngineService/MultiSubscribeHedgeQuote',
                request_serializer=stream__engine__server__pb2.SubscribeQuoteReq.SerializeToString,
                response_deserializer=stream__engine__server__pb2.MultiQuoteData.FromString,
                )
        self.SetParams = channel.unary_unary(
                '/trade.service.v1.StreamEngineService/SetParams',
                request_serializer=stream__engine__server__pb2.SetParamsReq.SerializeToString,
                response_deserializer=stream__engine__server__pb2.SetParamsResp.FromString,
                )


class StreamEngineServiceServicer(object):
    """Trade grpc
    """

    def GetQuote(self, request, context):
        """获取单次行情
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def SubscribeOneQuote(self, request, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def MultiSubscribeQuote(self, request, context):
        """订阅聚合行情
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def MultiSubscribeHedgeQuote(self, request, context):
        """订阅用于内部对冲的聚合行情
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def SetParams(self, request, context):
        """设置参数
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')


def add_StreamEngineServiceServicer_to_server(servicer, server):
    rpc_method_handlers = {
            'GetQuote': grpc.unary_unary_rpc_method_handler(
                    servicer.GetQuote,
                    request_deserializer=stream__engine__server__pb2.GetQuoteReq.FromString,
                    response_serializer=stream__engine__server__pb2.QuoteData.SerializeToString,
            ),
            'SubscribeOneQuote': grpc.unary_stream_rpc_method_handler(
                    servicer.SubscribeOneQuote,
                    request_deserializer=stream__engine__server__pb2.GetQuoteReq.FromString,
                    response_serializer=stream__engine__server__pb2.MultiQuoteData.SerializeToString,
            ),
            'MultiSubscribeQuote': grpc.unary_stream_rpc_method_handler(
                    servicer.MultiSubscribeQuote,
                    request_deserializer=stream__engine__server__pb2.SubscribeQuoteReq.FromString,
                    response_serializer=stream__engine__server__pb2.MultiQuoteData.SerializeToString,
            ),
            'MultiSubscribeHedgeQuote': grpc.unary_stream_rpc_method_handler(
                    servicer.MultiSubscribeHedgeQuote,
                    request_deserializer=stream__engine__server__pb2.SubscribeQuoteReq.FromString,
                    response_serializer=stream__engine__server__pb2.MultiQuoteData.SerializeToString,
            ),
            'SetParams': grpc.unary_unary_rpc_method_handler(
                    servicer.SetParams,
                    request_deserializer=stream__engine__server__pb2.SetParamsReq.FromString,
                    response_serializer=stream__engine__server__pb2.SetParamsResp.SerializeToString,
            ),
    }
    generic_handler = grpc.method_handlers_generic_handler(
            'trade.service.v1.StreamEngineService', rpc_method_handlers)
    server.add_generic_rpc_handlers((generic_handler,))


 # This class is part of an EXPERIMENTAL API.
class StreamEngineService(object):
    """Trade grpc
    """

    @staticmethod
    def GetQuote(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/trade.service.v1.StreamEngineService/GetQuote',
            stream__engine__server__pb2.GetQuoteReq.SerializeToString,
            stream__engine__server__pb2.QuoteData.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def SubscribeOneQuote(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_stream(request, target, '/trade.service.v1.StreamEngineService/SubscribeOneQuote',
            stream__engine__server__pb2.GetQuoteReq.SerializeToString,
            stream__engine__server__pb2.MultiQuoteData.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def MultiSubscribeQuote(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_stream(request, target, '/trade.service.v1.StreamEngineService/MultiSubscribeQuote',
            stream__engine__server__pb2.SubscribeQuoteReq.SerializeToString,
            stream__engine__server__pb2.MultiQuoteData.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def MultiSubscribeHedgeQuote(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_stream(request, target, '/trade.service.v1.StreamEngineService/MultiSubscribeHedgeQuote',
            stream__engine__server__pb2.SubscribeQuoteReq.SerializeToString,
            stream__engine__server__pb2.MultiQuoteData.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def SetParams(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/trade.service.v1.StreamEngineService/SetParams',
            stream__engine__server__pb2.SetParamsReq.SerializeToString,
            stream__engine__server__pb2.SetParamsResp.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)
