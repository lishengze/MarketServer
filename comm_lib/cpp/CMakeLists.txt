CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

SET("SO_NAME" bcts_comm)

PROJECT(${SO_NAME})

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
# for protos
INCLUDE_DIRECTORIES("../../protos/cpp")
# for include pandora
INCLUDE_DIRECTORIES("../../component")
INCLUDE_DIRECTORIES("/usr/local/include/nacos/")

AUX_SOURCE_DIRECTORY(. DIR_SRCS)
AUX_SOURCE_DIRECTORY(./Log LOG_SRCS)
AUX_SOURCE_DIRECTORY(./util UTIL_SRCS)
AUX_SOURCE_DIRECTORY(../../base/cpp BASE_SRCS)
AUX_SOURCE_DIRECTORY(../../protos/cpp PROTO_SRCS)

# for grpc
# INCLUDE_DIRECTORIES("/root/.local/include")
# for gogo
# INCLUDE_DIRECTORIES("/home/mk/go/src")
# for search lib pandora

# for search lib grpc
# LINK_DIRECTORIES("/root/.local/lib")

# INCLUDE_DIRECTORIES("/usr/local/include")

LINK_DIRECTORIES("/opt/utrade/console/lib/pandora/")
LINK_DIRECTORIES("/usr/local/lib")
LINK_DIRECTORIES("/usr/local/boost_1_68/lib")
LINK_DIRECTORIES("/usr/local/grpc/lib")

# ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/ ../../component ${CMAKE_CURRENT_BINARY_DIR}/component)

###############################################################################################
add_library(${SO_NAME} SHARED ${UTIL_SRCS} ${DIR_SRCS} ${LOG_SRCS} ${PROTO_SRCS} ${BASE_SRCS})

target_link_libraries(${SO_NAME} ${Boost_LIBRARIES})
target_link_libraries(${SO_NAME} boost_system)
target_link_libraries(${SO_NAME} pandora)
target_link_libraries(${SO_NAME} log4cplus)

target_link_libraries (${SO_NAME}  absl_synchronization absl_symbolize absl_stacktrace absl_debugging_internal absl_base)
target_link_libraries (${SO_NAME}  absl_strings absl_strings_internal absl_time absl_time_zone absl_exponential_biased)
target_link_libraries (${SO_NAME}  absl_spinlock_wait absl_log_severity absl_raw_logging_internal absl_throw_delegate)
target_link_libraries (${SO_NAME}  absl_malloc_internal absl_bad_optional_access absl_civil_time absl_int128)
target_link_libraries (${SO_NAME}  absl_demangle_internal absl_graphcycles_internal absl_hashtablez_sampler)
target_link_libraries (${SO_NAME}  absl_city absl_raw_hash_set absl_str_format_internal absl_cord absl_status)
target_link_libraries (${SO_NAME}  absl_bad_variant_access absl_hash absl_statusor)

# target_link_libraries(${SO_NAME} protobuf protoc protobuf-lite)
# target_link_libraries(${SO_NAME} grpc++_reflection grpc++ gpr ssl crypto)

target_link_libraries(${SO_NAME} protobuf )
target_link_libraries(${SO_NAME} grpc++)
target_link_libraries(${SO_NAME} gpr)

target_link_libraries(${SO_NAME} ssl crypto)

target_link_libraries(${SO_NAME} nacos-cli)
target_link_libraries(${SO_NAME} SQLiteCpp sqlite3 dl)

TARGET_LINK_LIBRARIES(${SO_NAME} rdkafka rdkafka++)

###############################################################################################
