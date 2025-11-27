cmake_minimum_required(VERSION 3.12)
project(data_server)

SET("BIN_NAME" data_server)

###############################################################################################
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g3 -Wall -pthread")
set(BOOST_ROOT /usr/local/boost_1_68)

find_package(Boost 1.68.0 COMPONENTS date_time filesystem system thread serialization math_tr1 chrono regex program_options unit_test_framework locale REQUIRED)
include_directories(${BOOST_INCLUDEDIR})

###############################################################################################
# for general include
include_directories("../..")
include_directories("../../lib/comm")
# for protos
include_directories("../../protos/cpp")
# for include pandora
include_directories("../../../component")
include_directories("/usr/local/include/nacos/")

aux_source_directory(. DIR_SRCS)
aux_source_directory(./config CONFIG_SRCS)
aux_source_directory(./Log LOG_SRCS)
aux_source_directory(./util UTIL_SRCS)
# aux_source_directory(./grpc_comm GRPC_COMM_SRCS)
aux_source_directory(./db_engine DB_ENGINE_SRCS)
aux_source_directory(./test TEST_SRCS)
aux_source_directory(../../base/cpp BASE_SRCS)

# aux_source_directory(../../protos/cpp PROTO_SRCS)


link_directories("/opt/utrade/console/lib/pandora/")
link_directories("../../lib/comm")

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
