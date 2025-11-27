cmake_minimum_required(VERSION 3.12)
project(TestRedis)

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
include_directories("../../base/include")
# for protos
include_directories("../../protos/cpp")
# for include pandora
include_directories("../../component")

include_directories("/usr/local/include/nacos/")

AUX_SOURCE_DIRECTORY(. DIR_SRCS)
AUX_SOURCE_DIRECTORY(./Log LOG_SRCS)
# AUX_SOURCE_DIRECTORY(./util UTIL_SRCS)
# AUX_SOURCE_DIRECTORY(../../base/cpp BASE_SRCS)
# AUX_SOURCE_DIRECTORY(../../protos/cpp PROTO_SRCS)

# for grpc
# include_directories("/root/.local/include")
# for gogo
# include_directories("/home/mk/go/src")
# for search lib pandora
link_directories("../../lib")
# for search lib grpc
# link_directories("/root/.local/lib")

# include_directories("/usr/local/include")

link_directories("/opt/utrade/console/lib/pandora/")
link_directories("/usr/local/grpc/lib/")

if (BUILD_DYNAMIC)
    message ("  build dynamic lib: yes")
else()
    message ("  build dynamic lib: no")
endif()

# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ ../../component ${CMAKE_CURRENT_BINARY_DIR}/component)

###############################################################################################
add_executable(TestRedis ${DIR_SRCS} ${LOG_SRCS})
if(DEFINED ENV{LOCAL})
    target_link_libraries(TestRedis asan ${Boost_LIBRARIES})
else()
    target_link_libraries(TestRedis ${Boost_LIBRARIES})
endif()

target_link_libraries(TestRedis pandora)
target_link_libraries(TestRedis log4cplus)

# target_link_libraries(TestRedis absl_synchronization)
# target_link_libraries(TestRedis protobuf protoc protobuf-lite)
# target_link_libraries(TestRedis grpc++_reflection grpc++ gpr ssl crypto)

# target_link_libraries(TestRedis nacos-cli)
# target_link_libraries(TestRedis reporter)

# # target_link_libraries(TestRedis dl)

# target_link_libraries(TestRedis dl SQLiteCpp sqlite3)

# TARGET_LINK_LIBRARIES(TestRedis ssl crypto)




###############################################################################################
