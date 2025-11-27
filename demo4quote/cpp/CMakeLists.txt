cmake_minimum_required(VERSION 3.12)
project(demo4quote)

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

aux_source_directory(. DIR_SRCS)
aux_source_directory(./Log LOG_SRCS)
aux_source_directory(./util UTIL_SRCS)
aux_source_directory(../../base/cpp BASE_SRCS)
aux_source_directory(../../protos/cpp PROTO_SRCS)

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

# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ ../../component ${CMAKE_CURRENT_BINARY_DIR}/component)

###############################################################################################
add_executable(demo4quote ${UTIL_SRCS} ${DIR_SRCS} ${LOG_SRCS} ${PROTO_SRCS} ${BASE_SRCS})
if(DEFINED ENV{LOCAL})
    target_link_libraries(demo4quote asan ${Boost_LIBRARIES})
else()
    target_link_libraries(demo4quote ${Boost_LIBRARIES})
endif()
target_link_libraries(demo4quote pandora)
target_link_libraries(demo4quote log4cplus)

target_link_libraries(demo4quote absl_synchronization)
target_link_libraries(demo4quote protobuf protoc protobuf-lite)
target_link_libraries(demo4quote grpc++_reflection grpc++ gpr ssl crypto)

target_link_libraries(demo4quote nacos-cli)
target_link_libraries(demo4quote reporter)

# target_link_libraries(demo4quote dl)

target_link_libraries(demo4quote dl SQLiteCpp sqlite3)

TARGET_LINK_LIBRARIES(demo4quote ssl crypto)




###############################################################################################
