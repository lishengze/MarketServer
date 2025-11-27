cmake_minimum_required(VERSION 3.12)
project(test_client)

SET("BIN_NAME" test_client)


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
include_directories("/usr/local/include/nacos/")

AUX_SOURCE_DIRECTORY(. DIR_SRCS)
AUX_SOURCE_DIRECTORY(./Log LOG_SRCS)
AUX_SOURCE_DIRECTORY(../../base/cpp BASE_SRCS)
AUX_SOURCE_DIRECTORY(../../protos/cpp PROTO_SRCS)


link_directories("/opt/utrade/console/lib/pandora/")

###############################################################################################
add_executable(${BIN_NAME} ${UTIL_SRCS} ${GRPC_COMM_SRCS} ${DIR_SRCS} ${LOG_SRCS} ${TEST_SRCS} ${PROTO_SRCS} ${BASE_SRCS})
target_link_libraries(${BIN_NAME} ${Boost_LIBRARIES})
target_link_libraries(${BIN_NAME} pandora)
target_link_libraries(${BIN_NAME} log4cplus)
target_link_libraries(${BIN_NAME} nacos-cli)

# target_link_libraries(test_client protobuf protoc protobuf-lite)
# target_link_libraries(test_client grpc++_reflection grpc++ gpr ssl crypto)

target_link_libraries(${BIN_NAME} absl_synchronization)
target_link_libraries(${BIN_NAME} protobuf )
target_link_libraries(${BIN_NAME} grpc++)
target_link_libraries(${BIN_NAME} gpr)
target_link_libraries(${BIN_NAME} ssl crypto)

###############################################################################################
