cmake_minimum_required(VERSION 3.12)
project(test)

SET (BIN_NAME "test")

if(DEFINED ENV{LOCAL})
    MESSAGE(STATUS "defined env.LOCAL")
endif()
set(CMAKE_CXX_STANDARD 11)

###############################################################################################
if(DEFINED ENV{LOCAL})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pg -g3 -Wall -pthread")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g3 -Wall -pthread")
endif()
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
AUX_SOURCE_DIRECTORY(./Log LOG_SRCS)

# for grpc
# include_directories("/root/.local/include")
# for gogo
# include_directories("/home/mk/go/src")
# for search lib pandora
# link_directories("../../lib")
# for search lib grpc
# link_directories("/root/.local/lib")

# include_directories("/usr/local/include")

# link_directories("/opt/utrade/console/lib/pandora/")

# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ ../../component ${CMAKE_CURRENT_BINARY_DIR}/component)

###############################################################################################
# add_executable(${BIN_NAME} ${UTIL_SRCS} ${DIR_SRCS} ${LOG_SRCS} ${PROTO_SRCS} ${BASE_SRCS})

add_executable(${BIN_NAME} ${DIR_SRCS} ${LOG_SRCS})

if(DEFINED ENV{LOCAL})
    target_link_libraries(${BIN_NAME} asan ${Boost_LIBRARIES})
else()
    target_link_libraries(${BIN_NAME} ${Boost_LIBRARIES})
endif()

target_link_libraries(${BIN_NAME} pandora)
target_link_libraries(${BIN_NAME} log4cplus)
# target_link_libraries(${BIN_NAME} protobuf protoc protobuf-lite)
# target_link_libraries(${BIN_NAME} grpc++_reflection grpc++ gpr ssl crypto)
# target_link_libraries(${BIN_NAME} nacos-cli)
# target_link_libraries(${BIN_NAME} reporter)
# target_link_libraries(${BIN_NAME} SQLiteCpp sqlite3 dl)

# TARGET_LINK_LIBRARIES(${BIN_NAME} ssl crypto)

###############################################################################################
