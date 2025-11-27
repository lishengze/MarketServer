cmake_minimum_required(VERSION 3.12)
project(demo4hub)

###############################################################################################
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g3 -Wall -pthread")
set(BOOST_ROOT /usr/local/boost_1_68)

find_package(Boost 1.68.0 COMPONENTS date_time filesystem system thread serialization math_tr1 chrono regex program_options unit_test_framework locale REQUIRED)
include_directories(${BOOST_INCLUDEDIR})

###############################################################################################
# for general include
include_directories("../..")
# for base include
include_directories("../../base/cpp")
# for protos
include_directories("../../protos/cpp")
# for include pandora
include_directories("../../component")

AUX_SOURCE_DIRECTORY(. DIR_SRCS)
AUX_SOURCE_DIRECTORY(./Log LOG_SRCS)
AUX_SOURCE_DIRECTORY(../../protos/cpp PROTO_SRCS)

link_directories("/opt/utrade/console/lib/pandora")

###############################################################################################
add_library(demo4hub SHARED ${DIR_SRCS} ${PROTO_SRCS} ${LOG_SRCS})        
target_link_libraries(demo4hub ${Boost_LIBRARIES})
target_link_libraries(demo4hub pandora)
target_link_libraries(demo4hub log4cplus)
target_link_libraries(demo4hub protobuf protoc protobuf-lite)
target_link_libraries(demo4hub grpc++_reflection grpc++ gpr ssl crypto)
target_link_libraries(demo4hub nacos-cli)

###############################################################################################