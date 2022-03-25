CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(data_server)

SET("BIN_NAME" data_server)

###############################################################################################
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g3 -Wall -pthread")
set(BOOST_ROOT /usr/local/boost_1_68)

FIND_PACKAGE(Boost 1.68.0 COMPONENTS date_time filesystem system thread serialization math_tr1 chrono regex program_options unit_test_framework locale REQUIRED)
INCLUDE_DIRECTORIES(${BOOST_INCLUDEDIR})

###############################################################################################
# for general include
INCLUDE_DIRECTORIES("../..")
INCLUDE_DIRECTORIES("../../lib/comm")
# for protos
INCLUDE_DIRECTORIES("../../protos/cpp")
# for include pandora
INCLUDE_DIRECTORIES("../../component")
INCLUDE_DIRECTORIES("/usr/local/include/nacos/")

AUX_SOURCE_DIRECTORY(. DIR_SRCS)
AUX_SOURCE_DIRECTORY(./config CONFIG_SRCS)
AUX_SOURCE_DIRECTORY(./Log LOG_SRCS)
AUX_SOURCE_DIRECTORY(./util UTIL_SRCS)
# AUX_SOURCE_DIRECTORY(./grpc_comm GRPC_COMM_SRCS)
AUX_SOURCE_DIRECTORY(./db_engine DB_ENGINE_SRCS)
AUX_SOURCE_DIRECTORY(./test TEST_SRCS)
AUX_SOURCE_DIRECTORY(../../base/cpp BASE_SRCS)

# AUX_SOURCE_DIRECTORY(../../protos/cpp PROTO_SRCS)


LINK_DIRECTORIES("/opt/utrade/console/lib/pandora/")
LINK_DIRECTORIES("../../lib/comm")

###############################################################################################
# add_executable(${BIN_NAME} ${CONFIG_SRCS} ${DB_ENGINE_SRCS} ${UTIL_SRCS} ${GRPC_COMM_SRCS} ${DIR_SRCS} ${LOG_SRCS} ${TEST_SRCS} ${PROTO_SRCS} ${BASE_SRCS})
add_executable(${BIN_NAME} ${CONFIG_SRCS} ${DB_ENGINE_SRCS} ${UTIL_SRCS} 
               ${DIR_SRCS} ${LOG_SRCS} ${TEST_SRCS} ${BASE_SRCS})

target_link_libraries(${BIN_NAME} ${Boost_LIBRARIES})
target_link_libraries(${BIN_NAME} pandora)
target_link_libraries(${BIN_NAME} log4cplus)
target_link_libraries(${BIN_NAME} nacos-cli)

# target_link_libraries(${BIN_NAME} absl_synchronization)

# target_link_libraries(${BIN_NAME} protobuf)
# target_link_libraries(${BIN_NAME} protoc protobuf-lite)
# target_link_libraries(${BIN_NAME} grpc++_reflection grpc++ gpr ssl crypto)

# target_link_libraries (${BIN_NAME}  absl_synchronization absl_symbolize absl_stacktrace absl_debugging_internal absl_base)
# target_link_libraries (${BIN_NAME}  absl_strings absl_strings_internal absl_time absl_time_zone absl_exponential_biased)
# target_link_libraries (${BIN_NAME}  absl_spinlock_wait absl_log_severity absl_raw_logging_internal absl_throw_delegate)
# target_link_libraries (${BIN_NAME}  absl_malloc_internal absl_bad_optional_access absl_civil_time absl_int128)
# target_link_libraries (${BIN_NAME}  absl_demangle_internal absl_graphcycles_internal absl_hashtablez_sampler)
# target_link_libraries (${BIN_NAME}  absl_city absl_raw_hash_set absl_str_format_internal absl_cord absl_status)
# target_link_libraries (${BIN_NAME}  absl_bad_variant_access absl_hash absl_statusor)

target_link_libraries(${BIN_NAME} protobuf )
target_link_libraries(${BIN_NAME} grpc++)
target_link_libraries(${BIN_NAME} gpr)
# target_link_libraries(${BIN_NAME} ssl crypto)


TARGET_LINK_LIBRARIES(${BIN_NAME} mysqlcppconn dl z)
TARGET_LINK_LIBRARIES(${BIN_NAME} bcts_comm)

###############################################################################################
