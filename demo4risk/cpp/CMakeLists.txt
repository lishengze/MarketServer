CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(demo4risk)

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
INCLUDE_DIRECTORIES("/usr/local/include/nacos/")


AUX_SOURCE_DIRECTORY(. DIR_SRCS)
AUX_SOURCE_DIRECTORY(./Log LOG_SRCS)
AUX_SOURCE_DIRECTORY(./util UTIL_SRCS)
AUX_SOURCE_DIRECTORY(./data_struct DATA_STRUCT_SRCS)
AUX_SOURCE_DIRECTORY(../../base/cpp BASE_SRCS)
AUX_SOURCE_DIRECTORY(../../protos/cpp PROTO_SRCS)

# for include pandora
# INCLUDE_DIRECTORIES("../../pandora")
# for grpc
# INCLUDE_DIRECTORIES("/root/.local/include")
# for gogo
# INCLUDE_DIRECTORIES("/home/mk/go/src")
# for search lib pandora
# LINK_DIRECTORIES("../lib")
# for search lib grpc
LINK_DIRECTORIES("/opt/utrade/console/lib/pandora/")
LINK_DIRECTORIES("/usr/local/boost_1_68/lib/")


###############################################################################################
add_executable(demo4risk ${UTIL_SRCS} ${DATA_STRUCT_SRCS} ${DIR_SRCS} ${LOG_SRCS} ${PROTO_SRCS} ${BASE_SRCS})
target_link_libraries(demo4risk ${Boost_LIBRARIES})
target_link_libraries(demo4risk pandora)
target_link_libraries(demo4risk log4cplus)
target_link_libraries(demo4risk nacos-cli)
target_link_libraries(demo4risk absl_synchronization)
target_link_libraries(demo4risk protobuf protoc protobuf-lite)
target_link_libraries(demo4risk grpc++_reflection grpc++ gpr ssl crypto)

###############################################################################################
