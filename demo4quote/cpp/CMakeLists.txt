CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(demo4quote)

###############################################################################################
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g3 -Wall -pthread")
set(BOOST_ROOT /usr/local/boost_1_68)

FIND_PACKAGE(Boost 1.68.0 COMPONENTS date_time filesystem system thread serialization math_tr1 chrono regex program_options unit_test_framework locale REQUIRED)
INCLUDE_DIRECTORIES(${BOOST_INCLUDEDIR})

###############################################################################################
# for general include
INCLUDE_DIRECTORIES("../..")
# for protos
INCLUDE_DIRECTORIES("../../protos/cpp")
# for include pandora
INCLUDE_DIRECTORIES("../../component/pandora")

AUX_SOURCE_DIRECTORY(. DIR_SRCS)
AUX_SOURCE_DIRECTORY(../../protos/cpp PROTO_SRCS)

# for grpc
# INCLUDE_DIRECTORIES("/root/.local/include")
# for gogo
# INCLUDE_DIRECTORIES("/home/mk/go/src")
# for search lib pandora
# LINK_DIRECTORIES("../lib")
# for search lib grpc
# LINK_DIRECTORIES("/root/.local/lib")

# INCLUDE_DIRECTORIES("/usr/local/include")

LINK_DIRECTORIES("/opt/utrade/console/lib/pandora/")

# ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/ ../../component ${CMAKE_CURRENT_BINARY_DIR}/component)

###############################################################################################
add_executable(demo4quote ${DIR_SRCS} ${PROTO_SRCS})
target_link_libraries(demo4quote ${Boost_LIBRARIES})
target_link_libraries(demo4quote pandora)
target_link_libraries(demo4quote log4cplus)
target_link_libraries(demo4quote protobuf protoc protobuf-lite)
target_link_libraries(demo4quote grpc++_reflection grpc++ gpr ssl crypto)
target_link_libraries(demo4quote nacos-cli)

###############################################################################################
