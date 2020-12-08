mkdir -p protos/cpp
mkdir -p protos/python

# gogo.proro
protoc --proto_path=protos --cpp_out=protos/cpp gogo.proto
# python3 -m grpc_tools.protoc -I protos --python_out=protos/python gogo.proto

# quote_data.proto
protoc --proto_path=protos --cpp_out=protos/cpp quote_data.proto

# python3 -m grpc_tools.protoc -I protos --python_out=protos/python quote_data.proto

# stream_engine.proto
# generate protobuf
protoc --proto_path=protos --cpp_out=protos/cpp stream_engine.proto
# generate grpc 
protoc --proto_path=protos --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin --grpc_out=protos/cpp stream_engine.proto
# generate for # python3
# python3 -m grpc_tools.protoc -I protos --python_out=protos/python --grpc_python_out=protos/python stream_engine.proto


# risk_controller.proto
# generate protobuf
protoc --proto_path=protos --cpp_out=protos/cpp risk_controller.proto
# generate grpc 
protoc --proto_path=protos --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin --grpc_out=protos/cpp risk_controller.proto
# generate for # python3
# python3 -m grpc_tools.protoc -I protos --python_out=protos/python --grpc_python_out=protos/python risk_controller.proto

# account.proto
# generate protobuf
protoc --proto_path=protos --cpp_out=protos/cpp account.proto
# generate grpc 
protoc --proto_path=protos --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin --grpc_out=protos/cpp account.proto
# generate for # python3
# python3 -m grpc_tools.protoc -I protos --python_out=protos/python --grpc_python_out=protos/python account.proto

# kline_server.proto
# generate protobuf
# protoc --proto_path=protos --cpp_out=protos/cpp kline_server.proto
# generate grpc 
# protoc --proto_path=protos --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin --grpc_out=protos/cpp kline_server.proto
# generate for # python3
# python3 -m grpc_tools.protoc -I protos --python_out=protos/python --grpc_python_out=protos/python kline_server.proto
