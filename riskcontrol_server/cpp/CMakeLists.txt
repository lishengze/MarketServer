CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

SET("BIN_NAME" riskcontrol_server)

PROJECT(${BIN_NAME})

set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -g3 -Wall -pthread")

add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
set(BOOST_ROOT /usr/local/boost_1_68)

FIND_PACKAGE(Boost 1.68.0 COMPONENTS date_time system filesystem thread 
                                     serialization math_tr1 chrono regex 
                                     program_options unit_test_framework 
                                     locale REQUIRED)
                                     
INCLUDE_DIRECTORIES(${BOOST_INCLUDEDIR})

###############################################################################################
# for general include
INCLUDE_DIRECTORIES("../..")
INCLUDE_DIRECTORIES("../../base/include")
INCLUDE_DIRECTORIES("../../lib/comm")
# for protos
INCLUDE_DIRECTORIES("../../protos/cpp")
# for include pandora
INCLUDE_DIRECTORIES("../../component")

AUX_SOURCE_DIRECTORY(. DIR_SRCS)
AUX_SOURCE_DIRECTORY(./Log LOG_SRCS)
AUX_SOURCE_DIRECTORY(./util UTIL_SRCS)
AUX_SOURCE_DIRECTORY(../../base/cpp BASE_SRCS)
# AUX_SOURCE_DIRECTORY(../../protos/cpp PROTO_SRCS)

# for grpc
# INCLUDE_DIRECTORIES("/root/.local/include")
# for gogo
# INCLUDE_DIRECTORIES("/home/mk/go/src")
# for search lib pandora
LINK_DIRECTORIES("../../lib")
LINK_DIRECTORIES("../../lib/comm")
# for search lib grpc
# LINK_DIRECTORIES("/root/.local/lib")

# INCLUDE_DIRECTORIES("/usr/local/include")

LINK_DIRECTORIES("/opt/utrade/console/lib/pandora/")
LINK_DIRECTORIES("/usr/local/lib")
LINK_DIRECTORIES("/usr/local/boost_1_68/lib")

# ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/ ../../component ${CMAKE_CURRENT_BINARY_DIR}/component)

###############################################################################################
add_executable(${BIN_NAME} ${UTIL_SRCS} ${DIR_SRCS} ${LOG_SRCS} ${PROTO_SRCS} ${BASE_SRCS})

target_link_libraries(${BIN_NAME} ${Boost_LIBRARIES})
target_link_libraries(${BIN_NAME} boost_system)
target_link_libraries(${BIN_NAME} pandora)
target_link_libraries(${BIN_NAME} log4cplus)
target_link_libraries(${BIN_NAME} protobuf protoc protobuf-lite)
target_link_libraries(${BIN_NAME} grpc++_reflection grpc++ gpr ssl crypto)
target_link_libraries(${BIN_NAME} nacos-cli)
# target_link_libraries(${BIN_NAME} reporter)
# target_link_libraries(${BIN_NAME} SQLiteCpp sqlite3 dl)

# TARGET_LINK_LIBRARIES(${BIN_NAME} ssl crypto)
# TARGET_LINK_LIBRARIES(${BIN_NAME} rdkafka rdkafka++)
TARGET_LINK_LIBRARIES(${BIN_NAME} bcts_comm)

###############################################################################################
