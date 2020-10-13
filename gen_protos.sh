# stream_engine_server.proto
# generate protobuf
protoc --proto_path=/home/mk/go/src --proto_path=protos --cpp_out=demo4quote/cpp stream_engine_server.proto
# generate grpc 
protoc --proto_path=/home/mk/go/src --proto_path=protos --plugin=protoc-gen-grpc=/root/.local/bin/grpc_cpp_plugin --grpc_out=demo4quote/cpp stream_engine_server.proto
# generate for python3
python3 -m grpc_tools.protoc -I protos --python_out=demo4quote/python --grpc_python_out=demo4quote/python stream_engine_server.proto
# generate protobuf
protoc --proto_path=/home/mk/go/src --proto_path=protos --cpp_out=demo4risk/cpp stream_engine_server.proto
# generate grpc 
protoc --proto_path=/home/mk/go/src --proto_path=protos --plugin=protoc-gen-grpc=/root/.local/bin/grpc_cpp_plugin --grpc_out=demo4risk/cpp stream_engine_server.proto
# generate for python3
python3 -m grpc_tools.protoc -I protos --python_out=demo4risk/python --grpc_python_out=demo4risk/python stream_engine_server.proto



# api.proto
# generate protobuf
protoc --proto_path=/home/mk/go/src --proto_path=protos --cpp_out=demo4risk/cpp api.proto
# generate grpc 
protoc --proto_path=/home/mk/go/src --proto_path=protos --plugin=protoc-gen-grpc=/root/.local/bin/grpc_cpp_plugin --grpc_out=demo4risk/cpp api.proto
# generate for python3
python3 -m grpc_tools.protoc -I protos --python_out=demo4risk/python --grpc_python_out=demo4risk/python api.proto