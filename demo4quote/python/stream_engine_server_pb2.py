# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: stream_engine_server.proto
"""Generated protocol buffer code."""
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


import gogo_pb2 as gogo__pb2


DESCRIPTOR = _descriptor.FileDescriptor(
  name='stream_engine_server.proto',
  package='trade.service.v1',
  syntax='proto3',
  serialized_options=b'Z\002v1',
  create_key=_descriptor._internal_create_key,
  serialized_pb=b'\n\x1astream_engine_server.proto\x12\x10trade.service.v1\x1a\ngogo.proto\"h\n\x0cSetParamsReq\x12\r\n\x05\x64\x65pth\x18\x01 \x01(\x05\x12\x11\n\tfrequency\x18\x02 \x01(\x05\x12\x0f\n\x07precise\x18\x03 \x01(\x05\x12\x0e\n\x06symbol\x18\x04 \x01(\t\x12\x15\n\rraw_frequency\x18\x05 \x01(\x05\"\x0f\n\rSetParamsResp\"\x13\n\x11SubscribeQuoteReq\"_\n\x0bGetQuoteReq\x12*\n\x08\x65xchange\x18\x01 \x01(\tB\x18\xe2\xde\x1f\x08\x45xchange\xea\xde\x1f\x08\x65xchange\x12$\n\x06symbol\x18\x02 \x01(\tB\x14\xe2\xde\x1f\x06Symbol\xea\xde\x1f\x06symbol\"S\n\x0eMultiQuoteData\x12\x41\n\x06quotes\x18\x01 \x03(\x0b\x32\x1b.trade.service.v1.QuoteDataB\x14\xe2\xde\x1f\x06Quotes\xea\xde\x1f\x06quotes\"\x95\x03\n\tQuoteData\x12$\n\x06symbol\x18\x01 \x01(\tB\x14\xe2\xde\x1f\x06Symbol\xea\xde\x1f\x06symbol\x12&\n\x07msg_seq\x18\x02 \x01(\x03\x42\x15\xe2\xde\x1f\x06MsgSeq\xea\xde\x1f\x07msg_seq\x12\x32\n\x04time\x18\x03 \x01(\tB$\xe2\xde\x1f\x04Time\xea\xde\x1f\x04time\xfa\xde\x1f\x10wx/pkg/time.Time\x12\x46\n\x0btime_arrive\x18\x04 \x01(\tB1\xe2\xde\x1f\nTimeArrive\xea\xde\x1f\x0btime_arrive\xfa\xde\x1f\x10wx/pkg/time.Time\x12J\n\task_depth\x18\x05 \x03(\x0b\x32\x1c.trade.service.v1.DepthLevelB\x19\xe2\xde\x1f\x08\x41skDepth\xea\xde\x1f\task_depth\x12J\n\tbid_depth\x18\x06 \x03(\x0b\x32\x1c.trade.service.v1.DepthLevelB\x19\xe2\xde\x1f\x08\x42idDepth\xea\xde\x1f\tbid_depth\x12&\n\x07is_snap\x18\x07 \x01(\x08\x42\x15\xe2\xde\x1f\x06IsSnap\xea\xde\x1f\x07is_snap\"\x89\x01\n\nDepthLevel\x12<\n\x05price\x18\x01 \x01(\x0b\x32\x19.trade.service.v1.DecimalB\x12\xe2\xde\x1f\x05Price\xea\xde\x1f\x05price\x12=\n\x04\x64\x61ta\x18\x02 \x03(\x0b\x32\x1d.trade.service.v1.DepthVolumeB\x10\xe2\xde\x1f\x04\x44\x61ta\xea\xde\x1f\x04\x64\x61ta\"L\n\x07\x44\x65\x63imal\x12!\n\x05value\x18\x01 \x01(\x03\x42\x12\xe2\xde\x1f\x05Value\xea\xde\x1f\x05value\x12\x1e\n\x04\x62\x61se\x18\x02 \x01(\x05\x42\x10\xe2\xde\x1f\x04\x42\x61se\xea\xde\x1f\x04\x62\x61se\"_\n\x0b\x44\x65pthVolume\x12$\n\x06volume\x18\x01 \x01(\x01\x42\x14\xe2\xde\x1f\x06Volume\xea\xde\x1f\x06volume\x12*\n\x08\x65xchange\x18\x02 \x01(\tB\x18\xe2\xde\x1f\x08\x45xchange\xea\xde\x1f\x08\x65xchange2\xe3\x02\n\x13StreamEngineService\x12\x46\n\x08GetQuote\x12\x1d.trade.service.v1.GetQuoteReq\x1a\x1b.trade.service.v1.QuoteData\x12V\n\x11SubscribeOneQuote\x12\x1d.trade.service.v1.GetQuoteReq\x1a .trade.service.v1.MultiQuoteData0\x01\x12^\n\x13MultiSubscribeQuote\x12#.trade.service.v1.SubscribeQuoteReq\x1a .trade.service.v1.MultiQuoteData0\x01\x12L\n\tSetParams\x12\x1e.trade.service.v1.SetParamsReq\x1a\x1f.trade.service.v1.SetParamsRespB\x04Z\x02v1b\x06proto3'
  ,
  dependencies=[gogo__pb2.DESCRIPTOR,])




_SETPARAMSREQ = _descriptor.Descriptor(
  name='SetParamsReq',
  full_name='trade.service.v1.SetParamsReq',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='depth', full_name='trade.service.v1.SetParamsReq.depth', index=0,
      number=1, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='frequency', full_name='trade.service.v1.SetParamsReq.frequency', index=1,
      number=2, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='precise', full_name='trade.service.v1.SetParamsReq.precise', index=2,
      number=3, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='symbol', full_name='trade.service.v1.SetParamsReq.symbol', index=3,
      number=4, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='raw_frequency', full_name='trade.service.v1.SetParamsReq.raw_frequency', index=4,
      number=5, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=60,
  serialized_end=164,
)


_SETPARAMSRESP = _descriptor.Descriptor(
  name='SetParamsResp',
  full_name='trade.service.v1.SetParamsResp',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=166,
  serialized_end=181,
)


_SUBSCRIBEQUOTEREQ = _descriptor.Descriptor(
  name='SubscribeQuoteReq',
  full_name='trade.service.v1.SubscribeQuoteReq',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=183,
  serialized_end=202,
)


_GETQUOTEREQ = _descriptor.Descriptor(
  name='GetQuoteReq',
  full_name='trade.service.v1.GetQuoteReq',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='exchange', full_name='trade.service.v1.GetQuoteReq.exchange', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\010Exchange\352\336\037\010exchange', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='symbol', full_name='trade.service.v1.GetQuoteReq.symbol', index=1,
      number=2, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\006Symbol\352\336\037\006symbol', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=204,
  serialized_end=299,
)


_MULTIQUOTEDATA = _descriptor.Descriptor(
  name='MultiQuoteData',
  full_name='trade.service.v1.MultiQuoteData',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='quotes', full_name='trade.service.v1.MultiQuoteData.quotes', index=0,
      number=1, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\006Quotes\352\336\037\006quotes', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=301,
  serialized_end=384,
)


_QUOTEDATA = _descriptor.Descriptor(
  name='QuoteData',
  full_name='trade.service.v1.QuoteData',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='symbol', full_name='trade.service.v1.QuoteData.symbol', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\006Symbol\352\336\037\006symbol', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='msg_seq', full_name='trade.service.v1.QuoteData.msg_seq', index=1,
      number=2, type=3, cpp_type=2, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\006MsgSeq\352\336\037\007msg_seq', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='time', full_name='trade.service.v1.QuoteData.time', index=2,
      number=3, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\004Time\352\336\037\004time\372\336\037\020wx/pkg/time.Time', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='time_arrive', full_name='trade.service.v1.QuoteData.time_arrive', index=3,
      number=4, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\nTimeArrive\352\336\037\013time_arrive\372\336\037\020wx/pkg/time.Time', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='ask_depth', full_name='trade.service.v1.QuoteData.ask_depth', index=4,
      number=5, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\010AskDepth\352\336\037\task_depth', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='bid_depth', full_name='trade.service.v1.QuoteData.bid_depth', index=5,
      number=6, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\010BidDepth\352\336\037\tbid_depth', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='is_snap', full_name='trade.service.v1.QuoteData.is_snap', index=6,
      number=7, type=8, cpp_type=7, label=1,
      has_default_value=False, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\006IsSnap\352\336\037\007is_snap', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=387,
  serialized_end=792,
)


_DEPTHLEVEL = _descriptor.Descriptor(
  name='DepthLevel',
  full_name='trade.service.v1.DepthLevel',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='price', full_name='trade.service.v1.DepthLevel.price', index=0,
      number=1, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\005Price\352\336\037\005price', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='data', full_name='trade.service.v1.DepthLevel.data', index=1,
      number=2, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\004Data\352\336\037\004data', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=795,
  serialized_end=932,
)


_DECIMAL = _descriptor.Descriptor(
  name='Decimal',
  full_name='trade.service.v1.Decimal',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='value', full_name='trade.service.v1.Decimal.value', index=0,
      number=1, type=3, cpp_type=2, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\005Value\352\336\037\005value', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='base', full_name='trade.service.v1.Decimal.base', index=1,
      number=2, type=5, cpp_type=1, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\004Base\352\336\037\004base', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=934,
  serialized_end=1010,
)


_DEPTHVOLUME = _descriptor.Descriptor(
  name='DepthVolume',
  full_name='trade.service.v1.DepthVolume',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='volume', full_name='trade.service.v1.DepthVolume.volume', index=0,
      number=1, type=1, cpp_type=5, label=1,
      has_default_value=False, default_value=float(0),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\006Volume\352\336\037\006volume', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='exchange', full_name='trade.service.v1.DepthVolume.exchange', index=1,
      number=2, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\010Exchange\352\336\037\010exchange', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=1012,
  serialized_end=1107,
)

_MULTIQUOTEDATA.fields_by_name['quotes'].message_type = _QUOTEDATA
_QUOTEDATA.fields_by_name['ask_depth'].message_type = _DEPTHLEVEL
_QUOTEDATA.fields_by_name['bid_depth'].message_type = _DEPTHLEVEL
_DEPTHLEVEL.fields_by_name['price'].message_type = _DECIMAL
_DEPTHLEVEL.fields_by_name['data'].message_type = _DEPTHVOLUME
DESCRIPTOR.message_types_by_name['SetParamsReq'] = _SETPARAMSREQ
DESCRIPTOR.message_types_by_name['SetParamsResp'] = _SETPARAMSRESP
DESCRIPTOR.message_types_by_name['SubscribeQuoteReq'] = _SUBSCRIBEQUOTEREQ
DESCRIPTOR.message_types_by_name['GetQuoteReq'] = _GETQUOTEREQ
DESCRIPTOR.message_types_by_name['MultiQuoteData'] = _MULTIQUOTEDATA
DESCRIPTOR.message_types_by_name['QuoteData'] = _QUOTEDATA
DESCRIPTOR.message_types_by_name['DepthLevel'] = _DEPTHLEVEL
DESCRIPTOR.message_types_by_name['Decimal'] = _DECIMAL
DESCRIPTOR.message_types_by_name['DepthVolume'] = _DEPTHVOLUME
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

SetParamsReq = _reflection.GeneratedProtocolMessageType('SetParamsReq', (_message.Message,), {
  'DESCRIPTOR' : _SETPARAMSREQ,
  '__module__' : 'stream_engine_server_pb2'
  # @@protoc_insertion_point(class_scope:trade.service.v1.SetParamsReq)
  })
_sym_db.RegisterMessage(SetParamsReq)

SetParamsResp = _reflection.GeneratedProtocolMessageType('SetParamsResp', (_message.Message,), {
  'DESCRIPTOR' : _SETPARAMSRESP,
  '__module__' : 'stream_engine_server_pb2'
  # @@protoc_insertion_point(class_scope:trade.service.v1.SetParamsResp)
  })
_sym_db.RegisterMessage(SetParamsResp)

SubscribeQuoteReq = _reflection.GeneratedProtocolMessageType('SubscribeQuoteReq', (_message.Message,), {
  'DESCRIPTOR' : _SUBSCRIBEQUOTEREQ,
  '__module__' : 'stream_engine_server_pb2'
  # @@protoc_insertion_point(class_scope:trade.service.v1.SubscribeQuoteReq)
  })
_sym_db.RegisterMessage(SubscribeQuoteReq)

GetQuoteReq = _reflection.GeneratedProtocolMessageType('GetQuoteReq', (_message.Message,), {
  'DESCRIPTOR' : _GETQUOTEREQ,
  '__module__' : 'stream_engine_server_pb2'
  # @@protoc_insertion_point(class_scope:trade.service.v1.GetQuoteReq)
  })
_sym_db.RegisterMessage(GetQuoteReq)

MultiQuoteData = _reflection.GeneratedProtocolMessageType('MultiQuoteData', (_message.Message,), {
  'DESCRIPTOR' : _MULTIQUOTEDATA,
  '__module__' : 'stream_engine_server_pb2'
  # @@protoc_insertion_point(class_scope:trade.service.v1.MultiQuoteData)
  })
_sym_db.RegisterMessage(MultiQuoteData)

QuoteData = _reflection.GeneratedProtocolMessageType('QuoteData', (_message.Message,), {
  'DESCRIPTOR' : _QUOTEDATA,
  '__module__' : 'stream_engine_server_pb2'
  # @@protoc_insertion_point(class_scope:trade.service.v1.QuoteData)
  })
_sym_db.RegisterMessage(QuoteData)

DepthLevel = _reflection.GeneratedProtocolMessageType('DepthLevel', (_message.Message,), {
  'DESCRIPTOR' : _DEPTHLEVEL,
  '__module__' : 'stream_engine_server_pb2'
  # @@protoc_insertion_point(class_scope:trade.service.v1.DepthLevel)
  })
_sym_db.RegisterMessage(DepthLevel)

Decimal = _reflection.GeneratedProtocolMessageType('Decimal', (_message.Message,), {
  'DESCRIPTOR' : _DECIMAL,
  '__module__' : 'stream_engine_server_pb2'
  # @@protoc_insertion_point(class_scope:trade.service.v1.Decimal)
  })
_sym_db.RegisterMessage(Decimal)

DepthVolume = _reflection.GeneratedProtocolMessageType('DepthVolume', (_message.Message,), {
  'DESCRIPTOR' : _DEPTHVOLUME,
  '__module__' : 'stream_engine_server_pb2'
  # @@protoc_insertion_point(class_scope:trade.service.v1.DepthVolume)
  })
_sym_db.RegisterMessage(DepthVolume)


DESCRIPTOR._options = None
_GETQUOTEREQ.fields_by_name['exchange']._options = None
_GETQUOTEREQ.fields_by_name['symbol']._options = None
_MULTIQUOTEDATA.fields_by_name['quotes']._options = None
_QUOTEDATA.fields_by_name['symbol']._options = None
_QUOTEDATA.fields_by_name['msg_seq']._options = None
_QUOTEDATA.fields_by_name['time']._options = None
_QUOTEDATA.fields_by_name['time_arrive']._options = None
_QUOTEDATA.fields_by_name['ask_depth']._options = None
_QUOTEDATA.fields_by_name['bid_depth']._options = None
_QUOTEDATA.fields_by_name['is_snap']._options = None
_DEPTHLEVEL.fields_by_name['price']._options = None
_DEPTHLEVEL.fields_by_name['data']._options = None
_DECIMAL.fields_by_name['value']._options = None
_DECIMAL.fields_by_name['base']._options = None
_DEPTHVOLUME.fields_by_name['volume']._options = None
_DEPTHVOLUME.fields_by_name['exchange']._options = None

_STREAMENGINESERVICE = _descriptor.ServiceDescriptor(
  name='StreamEngineService',
  full_name='trade.service.v1.StreamEngineService',
  file=DESCRIPTOR,
  index=0,
  serialized_options=None,
  create_key=_descriptor._internal_create_key,
  serialized_start=1110,
  serialized_end=1465,
  methods=[
  _descriptor.MethodDescriptor(
    name='GetQuote',
    full_name='trade.service.v1.StreamEngineService.GetQuote',
    index=0,
    containing_service=None,
    input_type=_GETQUOTEREQ,
    output_type=_QUOTEDATA,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='SubscribeOneQuote',
    full_name='trade.service.v1.StreamEngineService.SubscribeOneQuote',
    index=1,
    containing_service=None,
    input_type=_GETQUOTEREQ,
    output_type=_MULTIQUOTEDATA,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='MultiSubscribeQuote',
    full_name='trade.service.v1.StreamEngineService.MultiSubscribeQuote',
    index=2,
    containing_service=None,
    input_type=_SUBSCRIBEQUOTEREQ,
    output_type=_MULTIQUOTEDATA,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='SetParams',
    full_name='trade.service.v1.StreamEngineService.SetParams',
    index=3,
    containing_service=None,
    input_type=_SETPARAMSREQ,
    output_type=_SETPARAMSRESP,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
])
_sym_db.RegisterServiceDescriptor(_STREAMENGINESERVICE)

DESCRIPTOR.services_by_name['StreamEngineService'] = _STREAMENGINESERVICE

# @@protoc_insertion_point(module_scope)
