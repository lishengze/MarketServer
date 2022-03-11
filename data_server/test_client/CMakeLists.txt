CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(test_client)


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
AUX_SOURCE_DIRECTORY(../../base/cpp BASE_SRCS)
AUX_SOURCE_DIRECTORY(../../protos/cpp PROTO_SRCS)


LINK_DIRECTORIES("/opt/utrade/console/lib/pandora/")

###############################################################################################
add_executable(test_client ${UTIL_SRCS} ${GRPC_COMM_SRCS} ${DIR_SRCS} ${LOG_SRCS} ${TEST_SRCS} ${PROTO_SRCS} ${BASE_SRCS})
target_link_libraries(test_client ${Boost_LIBRARIES})
target_link_libraries(test_client pandora)
target_link_libraries(test_client log4cplus)
target_link_libraries(test_client nacos-cli)
target_link_libraries(test_client protobuf protoc protobuf-lite)
target_link_libraries(test_client grpc++_reflection grpc++ gpr ssl crypto)

###############################################################################################
