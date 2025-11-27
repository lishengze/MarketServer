cmake_minimum_required(VERSION 3.12)
project(test)

###############################################################################################
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g3 -Wall -pthread")
set(BOOST_ROOT /usr/local/boost_1_68)

find_package(Boost 1.68.0 COMPONENTS date_time filesystem system thread serialization math_tr1 chrono regex program_options unit_test_framework locale REQUIRED)
include_directories(${BOOST_INCLUDEDIR})

###############################################################################################
# for general include
include_directories("../..")
# for protos
include_directories("../../protos/cpp")
# for include pandora
include_directories("../../component")

AUX_SOURCE_DIRECTORY(. DIR_SRCS)
AUX_SOURCE_DIRECTORY(../../base/cpp BASE_SRCS)
AUX_SOURCE_DIRECTORY(../../protos/cpp PROTO_SRCS)

# for include pandora
# include_directories("../../pandora")
# for grpc
# include_directories("/root/.local/include")
# for gogo
# include_directories("/home/mk/go/src")
# for search lib pandora
# link_directories("../lib")
# for search lib grpc
link_directories("/opt/utrade/console/lib/pandora/")

###############################################################################################
add_executable(test ${DIR_SRCS} ${PROTO_SRCS} ${BASE_SRCS})
target_link_libraries(test ${Boost_LIBRARIES})
target_link_libraries(test pandora)
target_link_libraries(test nacos-cli)
target_link_libraries(test protobuf protoc protobuf-lite)
target_link_libraries(test grpc++_reflection grpc++ gpr ssl crypto)

###############################################################################################
