cmake_minimum_required(VERSION 3.12)
project(test)
SET("BIN_NAME" test)

###############################################################################################
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g3 -Wall -pthread")
set(BOOST_ROOT /usr/local/boost_1_68)

find_package(Boost 1.68.0 COMPONENTS date_time filesystem system thread serialization math_tr1 chrono regex program_options unit_test_framework locale REQUIRED)
include_directories(${BOOST_INCLUDEDIR})

###############################################################################################
# for general include
include_directories("../..")
include_directories("../cpp")
# for base include
include_directories("../../base/cpp")
# for protos
include_directories("../../protos/cpp")
# for include pandora
include_directories("../../component")

AUX_SOURCE_DIRECTORY(../cpp TOOL_SRCS)
AUX_SOURCE_DIRECTORY(. DIR_SRCS)
AUX_SOURCE_DIRECTORY(../../protos/cpp PROTO_SRCS)

link_directories("/opt/utrade/console/lib/pandora")

###############################################################################################
ADD_EXECUTABLE(${BIN_NAME}  ${DIR_SRCS} ${TOOL_SRCS} ${PROTO_SRCS})        
target_link_libraries(${BIN_NAME} ${Boost_LIBRARIES})
target_link_libraries(${BIN_NAME} pandora)
target_link_libraries(${BIN_NAME} log4cplus)
target_link_libraries(${BIN_NAME} protobuf protoc protobuf-lite)
target_link_libraries(${BIN_NAME} grpc++_reflection grpc++ gpr ssl crypto)
target_link_libraries(${BIN_NAME} nacos-cli)

###############################################################################################