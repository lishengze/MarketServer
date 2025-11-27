cmake_minimum_required(VERSION 3.12)

SET("BIN_NAME" exchange_server)

project(${BIN_NAME})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -g3 -Wall -pthread")

add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
set(BOOST_ROOT /usr/local/boost_1_68)

find_package(Boost 1.68.0 COMPONENTS date_time system filesystem thread 
                                     serialization math_tr1 chrono regex 
                                     program_options unit_test_framework 
                                     locale REQUIRED)
                                     
include_directories(${BOOST_INCLUDEDIR})

###############################################################################################
# for general include
include_directories("../..")
include_directories("../../base/include")
include_directories("../../lib/comm")
# for protos
include_directories("../../protos/cpp")
# for include pandora
include_directories("../../component")

AUX_SOURCE_DIRECTORY(. DIR_SRCS)
AUX_SOURCE_DIRECTORY(./Log LOG_SRCS)
AUX_SOURCE_DIRECTORY(./util UTIL_SRCS)
AUX_SOURCE_DIRECTORY(../../base/cpp BASE_SRCS)
# AUX_SOURCE_DIRECTORY(../../protos/cpp PROTO_SRCS)

# for grpc
# include_directories("/root/.local/include")
# for gogo
# include_directories("/home/mk/go/src")
# for search lib pandora
link_directories("../../lib")
link_directories("../../lib/comm")
# for search lib grpc
# link_directories("/root/.local/lib")

# include_directories("/usr/local/include")

link_directories("/opt/utrade/console/lib/pandora/")
link_directories("/usr/local/lib")
link_directories("/usr/local/boost_1_68/lib")

# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ ../../component ${CMAKE_CURRENT_BINARY_DIR}/component)

###############################################################################################
add_executable(${BIN_NAME} ${UTIL_SRCS} ${DIR_SRCS} ${LOG_SRCS} ${PROTO_SRCS} ${BASE_SRCS})

target_link_libraries(${BIN_NAME} ${Boost_LIBRARIES})
target_link_libraries(${BIN_NAME} boost_system)
target_link_libraries(${BIN_NAME} pandora)
target_link_libraries(${BIN_NAME} log4cplus)
# target_link_libraries(${BIN_NAME} protobuf protoc protobuf-lite)
# target_link_libraries(${BIN_NAME} grpc++_reflection grpc++ gpr ssl crypto)
target_link_libraries(${BIN_NAME} nacos-cli)
# target_link_libraries(${BIN_NAME} reporter)
# target_link_libraries(${BIN_NAME} SQLiteCpp sqlite3 dl)

# TARGET_LINK_LIBRARIES(${BIN_NAME} ssl crypto)
# TARGET_LINK_LIBRARIES(${BIN_NAME} rdkafka rdkafka++)
TARGET_LINK_LIBRARIES(${BIN_NAME} bcts_comm)

###############################################################################################
