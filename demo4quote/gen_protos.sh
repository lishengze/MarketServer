# generate protobuf
protoc --proto_path=/home/mk/go/src --proto_path=protos --cpp_out=cpp api.proto

# generate grpc 
protoc --proto_path=/home/mk/go/src --proto_path=protos --plugin=protoc-gen-grpc=/root/.local/bin/grpc_cpp_plugin --grpc_out=cpp api.proto

# generate for python3
python3 -m grpc_tools.protoc -I protos --python_out=python --grpc_python_out=python api.proto

# generate protobuf
protoc --proto_path=/home/mk/go/src --proto_path=protos --cpp_out=cpp stream_engine_server.proto

# generate grpc 
protoc --proto_path=/home/mk/go/src --proto_path=protos --plugin=protoc-gen-grpc=/root/.local/bin/grpc_cpp_plugin --grpc_out=cpp stream_engine_server.proto

# generate for python3
python3 -m grpc_tools.protoc -I protos --python_out=python --grpc_python_out=python stream_engine_server.proto
