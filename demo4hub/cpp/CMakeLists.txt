CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(demo4hub)

###############################################################################################
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g3 -Wall -pthread")
set(BOOST_ROOT /usr/local/boost_1_68)

FIND_PACKAGE(Boost 1.68.0 COMPONENTS date_time filesystem system thread serialization math_tr1 chrono regex program_options unit_test_framework locale REQUIRED)
INCLUDE_DIRECTORIES(${BOOST_INCLUDEDIR})

###############################################################################################
# for general include
INCLUDE_DIRECTORIES("../..")
# for base include
INCLUDE_DIRECTORIES("../../base/cpp")
# for protos
INCLUDE_DIRECTORIES("../../protos/cpp")
# for include pandora
INCLUDE_DIRECTORIES("../../component/pandora")

AUX_SOURCE_DIRECTORY(. DIR_SRCS)
AUX_SOURCE_DIRECTORY(../../protos/cpp PROTO_SRCS)

###############################################################################################
add_library(demo4hub SHARED ${DIR_SRCS} ${PROTO_SRCS})        
target_link_libraries(demo4hub ${Boost_LIBRARIES})
target_link_libraries(demo4hub pandora)
target_link_libraries(demo4hub log4cplus)
target_link_libraries(demo4hub protobuf protoc protobuf-lite)
target_link_libraries(demo4hub grpc++_reflection grpc++ gpr ssl crypto)
target_link_libraries(demo4hub nacos-cli)

###############################################################################################