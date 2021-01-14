CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(test)
SET("BIN_NAME" test)

###############################################################################################
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g3 -Wall -pthread")
set(BOOST_ROOT /usr/local/boost_1_68)

FIND_PACKAGE(Boost 1.68.0 COMPONENTS date_time filesystem system thread serialization math_tr1 chrono regex program_options unit_test_framework locale REQUIRED)
INCLUDE_DIRECTORIES(${BOOST_INCLUDEDIR})

###############################################################################################
# for general include
INCLUDE_DIRECTORIES("../..")
INCLUDE_DIRECTORIES("../cpp")
# for base include
INCLUDE_DIRECTORIES("../../base/cpp")
# for protos
INCLUDE_DIRECTORIES("../../protos/cpp")
# for include pandora
INCLUDE_DIRECTORIES("../../component")

AUX_SOURCE_DIRECTORY(../cpp TOOL_SRCS)
AUX_SOURCE_DIRECTORY(. DIR_SRCS)
AUX_SOURCE_DIRECTORY(../../protos/cpp PROTO_SRCS)

LINK_DIRECTORIES("/opt/utrade/console/lib/pandora")

###############################################################################################
ADD_EXECUTABLE(${BIN_NAME}  ${DIR_SRCS} ${TOOL_SRCS} ${PROTO_SRCS})        
target_link_libraries(${BIN_NAME} ${Boost_LIBRARIES})
target_link_libraries(${BIN_NAME} pandora)
target_link_libraries(${BIN_NAME} log4cplus)
target_link_libraries(${BIN_NAME} protobuf protoc protobuf-lite)
target_link_libraries(${BIN_NAME} grpc++_reflection grpc++ gpr ssl crypto)
target_link_libraries(${BIN_NAME} nacos-cli)

###############################################################################################