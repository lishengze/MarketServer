# Generated by the gRPC Python protocol compiler plugin. DO NOT EDIT!
"""Client and server classes corresponding to protobuf-defined services."""
import grpc

import api_pb2 as api__pb2
import empty_pb2 as empty__pb2


class RiskControllerServiceStub(object):
    """Risk Controller API
    """

    def __init__(self, channel):
        """Constructor.

        Args:
            channel: A grpc.Channel.
        """
        self.ServeMarketStream4Broker = channel.unary_stream(
                '/quote.service.v1.RiskControllerService/ServeMarketStream4Broker',
                request_serializer=empty__pb2.Empty.SerializeToString,
                response_deserializer=api__pb2.MultiMarketStreamData.FromString,
                )
        self.ServeMarketStream4Hedge = channel.unary_stream(
                '/quote.service.v1.RiskControllerService/ServeMarketStream4Hedge',
                request_serializer=empty__pb2.Empty.SerializeToString,
                response_deserializer=api__pb2.MultiMarketStreamData.FromString,
                )
        self.ServeMarketStream4Client = channel.unary_stream(
                '/quote.service.v1.RiskControllerService/ServeMarketStream4Client',
                request_serializer=empty__pb2.Empty.SerializeToString,
                response_deserializer=api__pb2.MultiMarketStreamData.FromString,
                )
        self.PutOrderStream = channel.stream_unary(
                '/quote.service.v1.RiskControllerService/PutOrderStream',
                request_serializer=api__pb2.MultiOrderStreamData.SerializeToString,
                response_deserializer=empty__pb2.Empty.FromString,
                )


class RiskControllerServiceServicer(object):
    """Risk Controller API
    """

    def ServeMarketStream4Broker(self, request, context):
        """发布聚合行情（用于撮合）
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def ServeMarketStream4Hedge(self, request, context):
        """发布聚合行情（用于对冲）
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def ServeMarketStream4Client(self, request, context):
        """发布聚合行情（用于客户端显示）
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def PutOrderStream(self, request_iterator, context):
        """客户端 对冲订单推流
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')


def add_RiskControllerServiceServicer_to_server(servicer, server):
    rpc_method_handlers = {
            'ServeMarketStream4Broker': grpc.unary_stream_rpc_method_handler(
                    servicer.ServeMarketStream4Broker,
                    request_deserializer=empty__pb2.Empty.FromString,
                    response_serializer=api__pb2.MultiMarketStreamData.SerializeToString,
            ),
            'ServeMarketStream4Hedge': grpc.unary_stream_rpc_method_handler(
                    servicer.ServeMarketStream4Hedge,
                    request_deserializer=empty__pb2.Empty.FromString,
                    response_serializer=api__pb2.MultiMarketStreamData.SerializeToString,
            ),
            'ServeMarketStream4Client': grpc.unary_stream_rpc_method_handler(
                    servicer.ServeMarketStream4Client,
                    request_deserializer=empty__pb2.Empty.FromString,
                    response_serializer=api__pb2.MultiMarketStreamData.SerializeToString,
            ),
            'PutOrderStream': grpc.stream_unary_rpc_method_handler(
                    servicer.PutOrderStream,
                    request_deserializer=api__pb2.MultiOrderStreamData.FromString,
                    response_serializer=empty__pb2.Empty.SerializeToString,
            ),
    }
    generic_handler = grpc.method_handlers_generic_handler(
            'quote.service.v1.RiskControllerService', rpc_method_handlers)
    server.add_generic_rpc_handlers((generic_handler,))


 # This class is part of an EXPERIMENTAL API.
class RiskControllerService(object):
    """Risk Controller API
    """

    @staticmethod
    def ServeMarketStream4Broker(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_stream(request, target, '/quote.service.v1.RiskControllerService/ServeMarketStream4Broker',
            empty__pb2.Empty.SerializeToString,
            api__pb2.MultiMarketStreamData.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def ServeMarketStream4Hedge(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_stream(request, target, '/quote.service.v1.RiskControllerService/ServeMarketStream4Hedge',
            empty__pb2.Empty.SerializeToString,
            api__pb2.MultiMarketStreamData.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def ServeMarketStream4Client(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_stream(request, target, '/quote.service.v1.RiskControllerService/ServeMarketStream4Client',
            empty__pb2.Empty.SerializeToString,
            api__pb2.MultiMarketStreamData.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def PutOrderStream(request_iterator,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.stream_unary(request_iterator, target, '/quote.service.v1.RiskControllerService/PutOrderStream',
            api__pb2.MultiOrderStreamData.SerializeToString,
            empty__pb2.Empty.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)
