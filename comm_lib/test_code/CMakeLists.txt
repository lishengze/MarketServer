cmake_minimum_required(VERSION 3.12)

SET("BIN_NAME" test_code)

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
# for protos
include_directories("../../protos/cpp")
# for include pandora
include_directories("../../../component")

aux_source_directory(. DIR_SRCS)
aux_source_directory(../cpp BASE_SRCS)
aux_source_directory(../../protos/cpp PROTO_SRCS)

# for grpc
# include_directories("/root/.local/include")
# for gogo
# include_directories("/home/mk/go/src")
# for search lib pandora

# for search lib grpc
# link_directories("/root/.local/lib")

# include_directories("/usr/local/include")

link_directories("/opt/utrade/console/lib/pandora/")
link_directories("/usr/local/lib")
link_directories("/usr/local/boost_1_68/lib")

# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ ../../component ${CMAKE_CURRENT_BINARY_DIR}/component)

###############################################################################################
add_executable(${BIN_NAME} ${PROTO_SRCS}  ${DIR_SRCS} ${BASE_SRCS})

target_link_libraries(${BIN_NAME} ${Boost_LIBRARIES})
target_link_libraries(${BIN_NAME} pandora)
target_link_libraries(${BIN_NAME} log4cplus)

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
target_link_libraries(${BIN_NAME} ssl crypto)

target_link_libraries(${BIN_NAME} nacos-cli)
TARGET_LINK_LIBRARIES(${BIN_NAME} rdkafka rdkafka++)

###############################################################################################
