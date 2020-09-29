CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(demo4quote)

###############################################################################################
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g3 -Wall -pthread")
FIND_PACKAGE(Boost 1.68.0 COMPONENTS date_time filesystem system thread serialization math_tr1 chrono regex program_options unit_test_framework locale REQUIRED)
INCLUDE_DIRECTORIES(${BOOST_INCLUDEDIR})

###############################################################################################
# for general include
INCLUDE_DIRECTORIES("..")
# for include envdeclare.h
INCLUDE_DIRECTORIES("../pandora")
# for search lib pandora
LINK_DIRECTORIES("./")

add_executable(demo4quote main.cpp redis_hub.cpp snap_task.cpp)
target_link_libraries(demo4quote ${Boost_LIBRARIES})
target_link_libraries(demo4quote pandora)
