CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(data_server)


###############################################################################################
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g3 -Wall -pthread")
set(BOOST_ROOT /usr/local/boost_1_68)

FIND_PACKAGE(Boost 1.68.0 COMPONENTS date_time filesystem system thread serialization math_tr1 chrono regex program_options unit_test_framework locale REQUIRED)
INCLUDE_DIRECTORIES(${BOOST_INCLUDEDIR})

###############################################################################################
# for general include
INCLUDE_DIRECTORIES("../..")
# for protos
INCLUDE_DIRECTORIES("../../protos/cpp")
# for include pandora
INCLUDE_DIRECTORIES("../../component")

AUX_SOURCE_DIRECTORY(. DIR_SRCS)
AUX_SOURCE_DIRECTORY(./Log LOG_SRCS)
AUX_SOURCE_DIRECTORY(./util UTIL_SRCS)
AUX_SOURCE_DIRECTORY(./grpc_comm GRPC_COMM_SRCS)
AUX_SOURCE_DIRECTORY(../../base/cpp BASE_SRCS)
AUX_SOURCE_DIRECTORY(../../protos/cpp PROTO_SRCS)


LINK_DIRECTORIES("/opt/utrade/console/lib/pandora/")

###############################################################################################
add_executable(data_server ${UTIL_SRCS} ${GRPC_COMM_SRCS} ${DIR_SRCS} ${LOG_SRCS} ${PROTO_SRCS} ${BASE_SRCS})
target_link_libraries(data_server ${Boost_LIBRARIES})
target_link_libraries(data_server pandora)
target_link_libraries(data_server log4cplus)
target_link_libraries(data_server nacos-cli)
target_link_libraries(data_server protobuf protoc protobuf-lite)
target_link_libraries(data_server grpc++_reflection grpc++ gpr ssl crypto)

###############################################################################################
