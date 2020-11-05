# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: api.proto
"""Generated protocol buffer code."""
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


import gogo_pb2 as gogo__pb2
import empty_pb2 as empty__pb2


DESCRIPTOR = _descriptor.FileDescriptor(
  name='api.proto',
  package='broker.service.v1',
  syntax='proto3',
  serialized_options=b'Z\002v1',
  create_key=_descriptor._internal_create_key,
  serialized_pb=b'\n\tapi.proto\x12\x11\x62roker.service.v1\x1a\ngogo.proto\x1a\x0b\x65mpty.proto\"b\n\x15MultiMarketStreamData\x12I\n\x06quotes\x18\x01 \x03(\x0b\x32#.broker.service.v1.MarketStreamDataB\x14\xe2\xde\x1f\x06Quotes\xea\xde\x1f\x06quotes\"\x9d\x03\n\x10MarketStreamData\x12$\n\x06symbol\x18\x01 \x01(\tB\x14\xe2\xde\x1f\x06Symbol\xea\xde\x1f\x06symbol\x12&\n\x07msg_seq\x18\x02 \x01(\tB\x15\xe2\xde\x1f\x06MsgSeq\xea\xde\x1f\x07msg_seq\x12\x32\n\x04time\x18\x03 \x01(\x03\x42$\xe2\xde\x1f\x04Time\xea\xde\x1f\x04time\xfa\xde\x1f\x10wx/pkg/time.Time\x12\x46\n\x0btime_arrive\x18\x04 \x01(\x03\x42\x31\xe2\xde\x1f\nTimeArrive\xea\xde\x1f\x0btime_arrive\xfa\xde\x1f\x10wx/pkg/time.Time\x12I\n\nask_depths\x18\x05 \x03(\x0b\x32\x18.broker.service.v1.DepthB\x1b\xe2\xde\x1f\tAskDepths\xea\xde\x1f\nask_depths\x12I\n\nbid_depths\x18\x06 \x03(\x0b\x32\x18.broker.service.v1.DepthB\x1b\xe2\xde\x1f\tBidDepths\xea\xde\x1f\nbid_depths\x12)\n\x08is_cover\x18\x07 \x01(\x08\x42\x17\xe2\xde\x1f\x07IsCover\xea\xde\x1f\x08is_cover\"\xc1\x01\n\x05\x44\x65pth\x12!\n\x05price\x18\x01 \x01(\tB\x12\xe2\xde\x1f\x05Price\xea\xde\x1f\x05price\x12\x42\n\x04\x64\x61ta\x18\x02 \x03(\x0b\x32\".broker.service.v1.Depth.DataEntryB\x10\xe2\xde\x1f\x04\x44\x61ta\xea\xde\x1f\x04\x64\x61ta\x12$\n\x06volume\x18\x03 \x01(\x01\x42\x14\xe2\xde\x1f\x06Volume\xea\xde\x1f\x06volume\x1a+\n\tDataEntry\x12\x0b\n\x03key\x18\x01 \x01(\t\x12\r\n\x05value\x18\x02 \x01(\x01:\x02\x38\x01\"\x0c\n\nEmptyReply2\xb7\x01\n\x06\x42roker\x12Y\n\x11ServeMarketStream\x12\x16.google.protobuf.Empty\x1a(.broker.service.v1.MultiMarketStreamData\"\x00\x30\x01\x12R\n\x0fPutMarketStream\x12#.broker.service.v1.MarketStreamData\x1a\x16.google.protobuf.Empty\"\x00(\x01\x42\x04Z\x02v1b\x06proto3'
  ,
  dependencies=[gogo__pb2.DESCRIPTOR,empty__pb2.DESCRIPTOR,])




_MULTIMARKETSTREAMDATA = _descriptor.Descriptor(
  name='MultiMarketStreamData',
  full_name='broker.service.v1.MultiMarketStreamData',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='quotes', full_name='broker.service.v1.MultiMarketStreamData.quotes', index=0,
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
  serialized_start=57,
  serialized_end=155,
)


_MARKETSTREAMDATA = _descriptor.Descriptor(
  name='MarketStreamData',
  full_name='broker.service.v1.MarketStreamData',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='symbol', full_name='broker.service.v1.MarketStreamData.symbol', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\006Symbol\352\336\037\006symbol', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='msg_seq', full_name='broker.service.v1.MarketStreamData.msg_seq', index=1,
      number=2, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\006MsgSeq\352\336\037\007msg_seq', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='time', full_name='broker.service.v1.MarketStreamData.time', index=2,
      number=3, type=3, cpp_type=2, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\004Time\352\336\037\004time\372\336\037\020wx/pkg/time.Time', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='time_arrive', full_name='broker.service.v1.MarketStreamData.time_arrive', index=3,
      number=4, type=3, cpp_type=2, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\nTimeArrive\352\336\037\013time_arrive\372\336\037\020wx/pkg/time.Time', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='ask_depths', full_name='broker.service.v1.MarketStreamData.ask_depths', index=4,
      number=5, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\tAskDepths\352\336\037\nask_depths', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='bid_depths', full_name='broker.service.v1.MarketStreamData.bid_depths', index=5,
      number=6, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\tBidDepths\352\336\037\nbid_depths', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='is_cover', full_name='broker.service.v1.MarketStreamData.is_cover', index=6,
      number=7, type=8, cpp_type=7, label=1,
      has_default_value=False, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\007IsCover\352\336\037\010is_cover', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
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
  serialized_start=158,
  serialized_end=571,
)


_DEPTH_DATAENTRY = _descriptor.Descriptor(
  name='DataEntry',
  full_name='broker.service.v1.Depth.DataEntry',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='key', full_name='broker.service.v1.Depth.DataEntry.key', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='value', full_name='broker.service.v1.Depth.DataEntry.value', index=1,
      number=2, type=1, cpp_type=5, label=1,
      has_default_value=False, default_value=float(0),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=b'8\001',
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=724,
  serialized_end=767,
)

_DEPTH = _descriptor.Descriptor(
  name='Depth',
  full_name='broker.service.v1.Depth',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='price', full_name='broker.service.v1.Depth.price', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\005Price\352\336\037\005price', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='data', full_name='broker.service.v1.Depth.data', index=1,
      number=2, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\004Data\352\336\037\004data', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='volume', full_name='broker.service.v1.Depth.volume', index=2,
      number=3, type=1, cpp_type=5, label=1,
      has_default_value=False, default_value=float(0),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=b'\342\336\037\006Volume\352\336\037\006volume', file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[_DEPTH_DATAENTRY, ],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=574,
  serialized_end=767,
)


_EMPTYREPLY = _descriptor.Descriptor(
  name='EmptyReply',
  full_name='broker.service.v1.EmptyReply',
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
  serialized_start=769,
  serialized_end=781,
)

_MULTIMARKETSTREAMDATA.fields_by_name['quotes'].message_type = _MARKETSTREAMDATA
_MARKETSTREAMDATA.fields_by_name['ask_depths'].message_type = _DEPTH
_MARKETSTREAMDATA.fields_by_name['bid_depths'].message_type = _DEPTH
_DEPTH_DATAENTRY.containing_type = _DEPTH
_DEPTH.fields_by_name['data'].message_type = _DEPTH_DATAENTRY
DESCRIPTOR.message_types_by_name['MultiMarketStreamData'] = _MULTIMARKETSTREAMDATA
DESCRIPTOR.message_types_by_name['MarketStreamData'] = _MARKETSTREAMDATA
DESCRIPTOR.message_types_by_name['Depth'] = _DEPTH
DESCRIPTOR.message_types_by_name['EmptyReply'] = _EMPTYREPLY
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

MultiMarketStreamData = _reflection.GeneratedProtocolMessageType('MultiMarketStreamData', (_message.Message,), {
  'DESCRIPTOR' : _MULTIMARKETSTREAMDATA,
  '__module__' : 'api_pb2'
  # @@protoc_insertion_point(class_scope:broker.service.v1.MultiMarketStreamData)
  })
_sym_db.RegisterMessage(MultiMarketStreamData)

MarketStreamData = _reflection.GeneratedProtocolMessageType('MarketStreamData', (_message.Message,), {
  'DESCRIPTOR' : _MARKETSTREAMDATA,
  '__module__' : 'api_pb2'
  # @@protoc_insertion_point(class_scope:broker.service.v1.MarketStreamData)
  })
_sym_db.RegisterMessage(MarketStreamData)

Depth = _reflection.GeneratedProtocolMessageType('Depth', (_message.Message,), {

  'DataEntry' : _reflection.GeneratedProtocolMessageType('DataEntry', (_message.Message,), {
    'DESCRIPTOR' : _DEPTH_DATAENTRY,
    '__module__' : 'api_pb2'
    # @@protoc_insertion_point(class_scope:broker.service.v1.Depth.DataEntry)
    })
  ,
  'DESCRIPTOR' : _DEPTH,
  '__module__' : 'api_pb2'
  # @@protoc_insertion_point(class_scope:broker.service.v1.Depth)
  })
_sym_db.RegisterMessage(Depth)
_sym_db.RegisterMessage(Depth.DataEntry)

EmptyReply = _reflection.GeneratedProtocolMessageType('EmptyReply', (_message.Message,), {
  'DESCRIPTOR' : _EMPTYREPLY,
  '__module__' : 'api_pb2'
  # @@protoc_insertion_point(class_scope:broker.service.v1.EmptyReply)
  })
_sym_db.RegisterMessage(EmptyReply)


DESCRIPTOR._options = None
_MULTIMARKETSTREAMDATA.fields_by_name['quotes']._options = None
_MARKETSTREAMDATA.fields_by_name['symbol']._options = None
_MARKETSTREAMDATA.fields_by_name['msg_seq']._options = None
_MARKETSTREAMDATA.fields_by_name['time']._options = None
_MARKETSTREAMDATA.fields_by_name['time_arrive']._options = None
_MARKETSTREAMDATA.fields_by_name['ask_depths']._options = None
_MARKETSTREAMDATA.fields_by_name['bid_depths']._options = None
_MARKETSTREAMDATA.fields_by_name['is_cover']._options = None
_DEPTH_DATAENTRY._options = None
_DEPTH.fields_by_name['price']._options = None
_DEPTH.fields_by_name['data']._options = None
_DEPTH.fields_by_name['volume']._options = None

_BROKER = _descriptor.ServiceDescriptor(
  name='Broker',
  full_name='broker.service.v1.Broker',
  file=DESCRIPTOR,
  index=0,
  serialized_options=None,
  create_key=_descriptor._internal_create_key,
  serialized_start=784,
  serialized_end=967,
  methods=[
  _descriptor.MethodDescriptor(
    name='ServeMarketStream',
    full_name='broker.service.v1.Broker.ServeMarketStream',
    index=0,
    containing_service=None,
    input_type=empty__pb2._EMPTY,
    output_type=_MULTIMARKETSTREAMDATA,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='PutMarketStream',
    full_name='broker.service.v1.Broker.PutMarketStream',
    index=1,
    containing_service=None,
    input_type=_MARKETSTREAMDATA,
    output_type=empty__pb2._EMPTY,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
])
_sym_db.RegisterServiceDescriptor(_BROKER)

DESCRIPTOR.services_by_name['Broker'] = _BROKER

# @@protoc_insertion_point(module_scope)
