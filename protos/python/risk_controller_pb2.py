# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: risk_controller.proto
"""Generated protocol buffer code."""
from google.protobuf import descriptor as _descriptor
from google.protobuf import descriptor_pool as _descriptor_pool
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


import gogo_pb2 as gogo__pb2
from google.protobuf import empty_pb2 as google_dot_protobuf_dot_empty__pb2
import quote_data_pb2 as quote__data__pb2


DESCRIPTOR = _descriptor_pool.Default().AddSerializedFile(b'\n\x15risk_controller.proto\x12\x10quote.service.v1\x1a\ngogo.proto\x1a\x1bgoogle/protobuf/empty.proto\x1a\x10quote_data.proto\"\x83\x03\n\x15TradedOrderStreamData\x12$\n\x06symbol\x18\x01 \x01(\tB\x14\xe2\xde\x1f\x06Symbol\xea\xde\x1f\x06symbol\x12!\n\x05price\x18\x02 \x01(\tB\x12\xe2\xde\x1f\x05Price\xea\xde\x1f\x05price\x12\x35\n\x0corder_amount\x18\x03 \x01(\x01\x42\x1f\xe2\xde\x1f\x0bOrderAmount\xea\xde\x1f\x0corder_amount\x12`\n\tdirection\x18\x04 \x01(\x0e\x32\x31.quote.service.v1.TradedOrderStreamData.DirectionB\x1a\xe2\xde\x1f\tDirection\xea\xde\x1f\tdirection\x12\x32\n\x04time\x18\x05 \x01(\x03\x42$\xe2\xde\x1f\x04Time\xea\xde\x1f\x04time\xfa\xde\x1f\x10wx/pkg/time.Time\x12\x34\n\x06traded\x18\x06 \x01(\x08\x42$\xe2\xde\x1f\x04Time\xea\xde\x1f\x04time\xfa\xde\x1f\x10wx/pkg/time.Time\"\x1e\n\tDirection\x12\x07\n\x03\x42UY\x10\x00\x12\x08\n\x04SELL\x10\x01\"_\n\x14MultiOrderStreamData\x12G\n\x06orders\x18\x01 \x03(\x0b\x32!.quote.service.v1.OrderStreamDataB\x14\xe2\xde\x1f\x06Orders\xea\xde\x1f\x06orders\"\xff\x01\n\x0fOrderStreamData\x12$\n\x06symbol\x18\x01 \x01(\tB\x14\xe2\xde\x1f\x06Symbol\xea\xde\x1f\x06symbol\x12!\n\x05price\x18\x02 \x01(\tB\x12\xe2\xde\x1f\x05Price\xea\xde\x1f\x05price\x12\x35\n\x0corder_amount\x18\x03 \x01(\x01\x42\x1f\xe2\xde\x1f\x0bOrderAmount\xea\xde\x1f\x0corder_amount\x12\x38\n\rhedged_amount\x18\x04 \x01(\x01\x42!\xe2\xde\x1f\x0cHedgedAmount\xea\xde\x1f\rhedged_amount\x12\x32\n\x04time\x18\x05 \x01(\x03\x42$\xe2\xde\x1f\x04Time\xea\xde\x1f\x04time\xfa\xde\x1f\x10wx/pkg/time.Time\"\xff\x01\n\x0cQuoteRequest\x12$\n\x06symbol\x18\x01 \x01(\tB\x14\xe2\xde\x1f\x06Symbol\xea\xde\x1f\x06symbol\x12$\n\x06\x61mount\x18\x02 \x01(\x01\x42\x14\xe2\xde\x1f\x06\x41mount\xea\xde\x1f\x06\x61mount\x12*\n\x08turnover\x18\x03 \x01(\x01\x42\x18\xe2\xde\x1f\x08Turnover\xea\xde\x1f\x08turnover\x12W\n\tdirection\x18\x04 \x01(\x0e\x32(.quote.service.v1.QuoteRequest.DirectionB\x1a\xe2\xde\x1f\tDirection\xea\xde\x1f\tdirection\"\x1e\n\tDirection\x12\x07\n\x03\x42UY\x10\x00\x12\x08\n\x04SELL\x10\x01\"\x8d\x02\n\rQuoteResponse\x12$\n\x06symbol\x18\x01 \x01(\tB\x14\xe2\xde\x1f\x06Symbol\xea\xde\x1f\x06symbol\x12!\n\x05price\x18\x02 \x01(\tB\x12\xe2\xde\x1f\x05Price\xea\xde\x1f\x05price\x12L\n\x06result\x18\x03 \x01(\x0e\x32&.quote.service.v1.QuoteResponse.ResultB\x14\xe2\xde\x1f\x06Result\xea\xde\x1f\x06result\"e\n\x06Result\x12\x06\n\x02OK\x10\x00\x12\x10\n\x0cWRONG_SYMBOL\x10\x01\x12\x13\n\x0fWRONG_DIRECTION\x10\x02\x12\x15\n\x11NOT_ENOUGH_VOLUME\x10\x03\x12\x15\n\x11NOT_ENOUGH_AMOUNT\x10\x04\"\xa5\x03\n\x11GetParamsResponse\x12G\n\nwatermarks\x18\x01 \x03(\x0b\x32\x33.quote.service.v1.GetParamsResponse.WatermarksEntry\x12\x43\n\x08\x61\x63\x63ounts\x18\x02 \x03(\x0b\x32\x31.quote.service.v1.GetParamsResponse.AccountsEntry\x12M\n\rconfiguration\x18\x03 \x03(\x0b\x32\x36.quote.service.v1.GetParamsResponse.ConfigurationEntry\x1aL\n\x0fWatermarksEntry\x12\x0b\n\x03key\x18\x01 \x01(\t\x12(\n\x05value\x18\x02 \x01(\x0b\x32\x19.quote.service.v1.Decimal:\x02\x38\x01\x1a/\n\rAccountsEntry\x12\x0b\n\x03key\x18\x01 \x01(\t\x12\r\n\x05value\x18\x02 \x01(\x01:\x02\x38\x01\x1a\x34\n\x12\x43onfigurationEntry\x12\x0b\n\x03key\x18\x01 \x01(\t\x12\r\n\x05value\x18\x02 \x01(\t:\x02\x38\x01\x32\x8b\x05\n\x0eRiskController\x12_\n\x18ServeMarketStream4Broker\x12\x16.google.protobuf.Empty\x1a\'.quote.service.v1.MultiMarketStreamData\"\x00\x30\x01\x12^\n\x17ServeMarketStream4Hedge\x12\x16.google.protobuf.Empty\x1a\'.quote.service.v1.MultiMarketStreamData\"\x00\x30\x01\x12j\n\x18ServeMarketStream4Client\x12\x16.google.protobuf.Empty\x1a\x32.quote.service.v1.MultiMarketStreamDataWithDecimal\"\x00\x30\x01\x12T\n\x0ePutOrderStream\x12&.quote.service.v1.MultiOrderStreamData\x1a\x16.google.protobuf.Empty\"\x00(\x01\x12M\n\x08OtcQuote\x12\x1e.quote.service.v1.QuoteRequest\x1a\x1f.quote.service.v1.QuoteResponse\"\x00\x12J\n\tGetParams\x12\x16.google.protobuf.Empty\x1a#.quote.service.v1.GetParamsResponse\"\x00\x12[\n\x14PutTradedOrderStream\x12\'.quote.service.v1.TradedOrderStreamData\x1a\x16.google.protobuf.Empty\"\x00(\x01\x62\x06proto3')



_TRADEDORDERSTREAMDATA = DESCRIPTOR.message_types_by_name['TradedOrderStreamData']
_MULTIORDERSTREAMDATA = DESCRIPTOR.message_types_by_name['MultiOrderStreamData']
_ORDERSTREAMDATA = DESCRIPTOR.message_types_by_name['OrderStreamData']
_QUOTEREQUEST = DESCRIPTOR.message_types_by_name['QuoteRequest']
_QUOTERESPONSE = DESCRIPTOR.message_types_by_name['QuoteResponse']
_GETPARAMSRESPONSE = DESCRIPTOR.message_types_by_name['GetParamsResponse']
_GETPARAMSRESPONSE_WATERMARKSENTRY = _GETPARAMSRESPONSE.nested_types_by_name['WatermarksEntry']
_GETPARAMSRESPONSE_ACCOUNTSENTRY = _GETPARAMSRESPONSE.nested_types_by_name['AccountsEntry']
_GETPARAMSRESPONSE_CONFIGURATIONENTRY = _GETPARAMSRESPONSE.nested_types_by_name['ConfigurationEntry']
_TRADEDORDERSTREAMDATA_DIRECTION = _TRADEDORDERSTREAMDATA.enum_types_by_name['Direction']
_QUOTEREQUEST_DIRECTION = _QUOTEREQUEST.enum_types_by_name['Direction']
_QUOTERESPONSE_RESULT = _QUOTERESPONSE.enum_types_by_name['Result']
TradedOrderStreamData = _reflection.GeneratedProtocolMessageType('TradedOrderStreamData', (_message.Message,), {
  'DESCRIPTOR' : _TRADEDORDERSTREAMDATA,
  '__module__' : 'risk_controller_pb2'
  # @@protoc_insertion_point(class_scope:quote.service.v1.TradedOrderStreamData)
  })
_sym_db.RegisterMessage(TradedOrderStreamData)

MultiOrderStreamData = _reflection.GeneratedProtocolMessageType('MultiOrderStreamData', (_message.Message,), {
  'DESCRIPTOR' : _MULTIORDERSTREAMDATA,
  '__module__' : 'risk_controller_pb2'
  # @@protoc_insertion_point(class_scope:quote.service.v1.MultiOrderStreamData)
  })
_sym_db.RegisterMessage(MultiOrderStreamData)

OrderStreamData = _reflection.GeneratedProtocolMessageType('OrderStreamData', (_message.Message,), {
  'DESCRIPTOR' : _ORDERSTREAMDATA,
  '__module__' : 'risk_controller_pb2'
  # @@protoc_insertion_point(class_scope:quote.service.v1.OrderStreamData)
  })
_sym_db.RegisterMessage(OrderStreamData)

QuoteRequest = _reflection.GeneratedProtocolMessageType('QuoteRequest', (_message.Message,), {
  'DESCRIPTOR' : _QUOTEREQUEST,
  '__module__' : 'risk_controller_pb2'
  # @@protoc_insertion_point(class_scope:quote.service.v1.QuoteRequest)
  })
_sym_db.RegisterMessage(QuoteRequest)

QuoteResponse = _reflection.GeneratedProtocolMessageType('QuoteResponse', (_message.Message,), {
  'DESCRIPTOR' : _QUOTERESPONSE,
  '__module__' : 'risk_controller_pb2'
  # @@protoc_insertion_point(class_scope:quote.service.v1.QuoteResponse)
  })
_sym_db.RegisterMessage(QuoteResponse)

GetParamsResponse = _reflection.GeneratedProtocolMessageType('GetParamsResponse', (_message.Message,), {

  'WatermarksEntry' : _reflection.GeneratedProtocolMessageType('WatermarksEntry', (_message.Message,), {
    'DESCRIPTOR' : _GETPARAMSRESPONSE_WATERMARKSENTRY,
    '__module__' : 'risk_controller_pb2'
    # @@protoc_insertion_point(class_scope:quote.service.v1.GetParamsResponse.WatermarksEntry)
    })
  ,

  'AccountsEntry' : _reflection.GeneratedProtocolMessageType('AccountsEntry', (_message.Message,), {
    'DESCRIPTOR' : _GETPARAMSRESPONSE_ACCOUNTSENTRY,
    '__module__' : 'risk_controller_pb2'
    # @@protoc_insertion_point(class_scope:quote.service.v1.GetParamsResponse.AccountsEntry)
    })
  ,

  'ConfigurationEntry' : _reflection.GeneratedProtocolMessageType('ConfigurationEntry', (_message.Message,), {
    'DESCRIPTOR' : _GETPARAMSRESPONSE_CONFIGURATIONENTRY,
    '__module__' : 'risk_controller_pb2'
    # @@protoc_insertion_point(class_scope:quote.service.v1.GetParamsResponse.ConfigurationEntry)
    })
  ,
  'DESCRIPTOR' : _GETPARAMSRESPONSE,
  '__module__' : 'risk_controller_pb2'
  # @@protoc_insertion_point(class_scope:quote.service.v1.GetParamsResponse)
  })
_sym_db.RegisterMessage(GetParamsResponse)
_sym_db.RegisterMessage(GetParamsResponse.WatermarksEntry)
_sym_db.RegisterMessage(GetParamsResponse.AccountsEntry)
_sym_db.RegisterMessage(GetParamsResponse.ConfigurationEntry)

_RISKCONTROLLER = DESCRIPTOR.services_by_name['RiskController']
if _descriptor._USE_C_DESCRIPTORS == False:

  DESCRIPTOR._options = None
  _TRADEDORDERSTREAMDATA.fields_by_name['symbol']._options = None
  _TRADEDORDERSTREAMDATA.fields_by_name['symbol']._serialized_options = b'\342\336\037\006Symbol\352\336\037\006symbol'
  _TRADEDORDERSTREAMDATA.fields_by_name['price']._options = None
  _TRADEDORDERSTREAMDATA.fields_by_name['price']._serialized_options = b'\342\336\037\005Price\352\336\037\005price'
  _TRADEDORDERSTREAMDATA.fields_by_name['order_amount']._options = None
  _TRADEDORDERSTREAMDATA.fields_by_name['order_amount']._serialized_options = b'\342\336\037\013OrderAmount\352\336\037\014order_amount'
  _TRADEDORDERSTREAMDATA.fields_by_name['direction']._options = None
  _TRADEDORDERSTREAMDATA.fields_by_name['direction']._serialized_options = b'\342\336\037\tDirection\352\336\037\tdirection'
  _TRADEDORDERSTREAMDATA.fields_by_name['time']._options = None
  _TRADEDORDERSTREAMDATA.fields_by_name['time']._serialized_options = b'\342\336\037\004Time\352\336\037\004time\372\336\037\020wx/pkg/time.Time'
  _TRADEDORDERSTREAMDATA.fields_by_name['traded']._options = None
  _TRADEDORDERSTREAMDATA.fields_by_name['traded']._serialized_options = b'\342\336\037\004Time\352\336\037\004time\372\336\037\020wx/pkg/time.Time'
  _MULTIORDERSTREAMDATA.fields_by_name['orders']._options = None
  _MULTIORDERSTREAMDATA.fields_by_name['orders']._serialized_options = b'\342\336\037\006Orders\352\336\037\006orders'
  _ORDERSTREAMDATA.fields_by_name['symbol']._options = None
  _ORDERSTREAMDATA.fields_by_name['symbol']._serialized_options = b'\342\336\037\006Symbol\352\336\037\006symbol'
  _ORDERSTREAMDATA.fields_by_name['price']._options = None
  _ORDERSTREAMDATA.fields_by_name['price']._serialized_options = b'\342\336\037\005Price\352\336\037\005price'
  _ORDERSTREAMDATA.fields_by_name['order_amount']._options = None
  _ORDERSTREAMDATA.fields_by_name['order_amount']._serialized_options = b'\342\336\037\013OrderAmount\352\336\037\014order_amount'
  _ORDERSTREAMDATA.fields_by_name['hedged_amount']._options = None
  _ORDERSTREAMDATA.fields_by_name['hedged_amount']._serialized_options = b'\342\336\037\014HedgedAmount\352\336\037\rhedged_amount'
  _ORDERSTREAMDATA.fields_by_name['time']._options = None
  _ORDERSTREAMDATA.fields_by_name['time']._serialized_options = b'\342\336\037\004Time\352\336\037\004time\372\336\037\020wx/pkg/time.Time'
  _QUOTEREQUEST.fields_by_name['symbol']._options = None
  _QUOTEREQUEST.fields_by_name['symbol']._serialized_options = b'\342\336\037\006Symbol\352\336\037\006symbol'
  _QUOTEREQUEST.fields_by_name['amount']._options = None
  _QUOTEREQUEST.fields_by_name['amount']._serialized_options = b'\342\336\037\006Amount\352\336\037\006amount'
  _QUOTEREQUEST.fields_by_name['turnover']._options = None
  _QUOTEREQUEST.fields_by_name['turnover']._serialized_options = b'\342\336\037\010Turnover\352\336\037\010turnover'
  _QUOTEREQUEST.fields_by_name['direction']._options = None
  _QUOTEREQUEST.fields_by_name['direction']._serialized_options = b'\342\336\037\tDirection\352\336\037\tdirection'
  _QUOTERESPONSE.fields_by_name['symbol']._options = None
  _QUOTERESPONSE.fields_by_name['symbol']._serialized_options = b'\342\336\037\006Symbol\352\336\037\006symbol'
  _QUOTERESPONSE.fields_by_name['price']._options = None
  _QUOTERESPONSE.fields_by_name['price']._serialized_options = b'\342\336\037\005Price\352\336\037\005price'
  _QUOTERESPONSE.fields_by_name['result']._options = None
  _QUOTERESPONSE.fields_by_name['result']._serialized_options = b'\342\336\037\006Result\352\336\037\006result'
  _GETPARAMSRESPONSE_WATERMARKSENTRY._options = None
  _GETPARAMSRESPONSE_WATERMARKSENTRY._serialized_options = b'8\001'
  _GETPARAMSRESPONSE_ACCOUNTSENTRY._options = None
  _GETPARAMSRESPONSE_ACCOUNTSENTRY._serialized_options = b'8\001'
  _GETPARAMSRESPONSE_CONFIGURATIONENTRY._options = None
  _GETPARAMSRESPONSE_CONFIGURATIONENTRY._serialized_options = b'8\001'
  _TRADEDORDERSTREAMDATA._serialized_start=103
  _TRADEDORDERSTREAMDATA._serialized_end=490
  _TRADEDORDERSTREAMDATA_DIRECTION._serialized_start=460
  _TRADEDORDERSTREAMDATA_DIRECTION._serialized_end=490
  _MULTIORDERSTREAMDATA._serialized_start=492
  _MULTIORDERSTREAMDATA._serialized_end=587
  _ORDERSTREAMDATA._serialized_start=590
  _ORDERSTREAMDATA._serialized_end=845
  _QUOTEREQUEST._serialized_start=848
  _QUOTEREQUEST._serialized_end=1103
  _QUOTEREQUEST_DIRECTION._serialized_start=460
  _QUOTEREQUEST_DIRECTION._serialized_end=490
  _QUOTERESPONSE._serialized_start=1106
  _QUOTERESPONSE._serialized_end=1375
  _QUOTERESPONSE_RESULT._serialized_start=1274
  _QUOTERESPONSE_RESULT._serialized_end=1375
  _GETPARAMSRESPONSE._serialized_start=1378
  _GETPARAMSRESPONSE._serialized_end=1799
  _GETPARAMSRESPONSE_WATERMARKSENTRY._serialized_start=1620
  _GETPARAMSRESPONSE_WATERMARKSENTRY._serialized_end=1696
  _GETPARAMSRESPONSE_ACCOUNTSENTRY._serialized_start=1698
  _GETPARAMSRESPONSE_ACCOUNTSENTRY._serialized_end=1745
  _GETPARAMSRESPONSE_CONFIGURATIONENTRY._serialized_start=1747
  _GETPARAMSRESPONSE_CONFIGURATIONENTRY._serialized_end=1799
  _RISKCONTROLLER._serialized_start=1802
  _RISKCONTROLLER._serialized_end=2453
# @@protoc_insertion_point(module_scope)
