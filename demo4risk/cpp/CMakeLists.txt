CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(demo4risk)

###############################################################################################
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g3 -Wall -pthread")
FIND_PACKAGE(Boost 1.68.0 COMPONENTS date_time filesystem system thread serialization math_tr1 chrono regex program_options unit_test_framework locale REQUIRED)
INCLUDE_DIRECTORIES(${BOOST_INCLUDEDIR})

###############################################################################################
# for general include
INCLUDE_DIRECTORIES("../..")
# for include pandora
INCLUDE_DIRECTORIES("../../pandora")
# for grpc
INCLUDE_DIRECTORIES("/root/.local/include")
# for gogo
INCLUDE_DIRECTORIES("/home/mk/go/src")
# for search lib pandora
LINK_DIRECTORIES("../lib")
# for search lib grpc
LINK_DIRECTORIES("/root/.local/lib")

###############################################################################################
add_executable(demo4risk main.cpp risk_controller.cpp datacenter.cpp grpc_server.cpp grpc_entity.cpp empty.pb.cc gogo.pb.cc api.pb.cc api.grpc.pb.cc stream_engine_server.grpc.pb.cc stream_engine_server.pb.cc)
target_link_libraries(demo4risk ${Boost_LIBRARIES})
target_link_libraries(demo4risk pandora)
target_link_libraries(demo4risk log4cplus)
target_link_libraries(demo4risk grpc++_reflection grpc++ gpr)

###############################################################################################