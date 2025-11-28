# copyright [2018] [wxblockchain]
# author: daniel.bian
# description: galaxy project main CMakeLists.txt
# update-date:
#   2019-01-04  init version
##############################################################################################
cmake_minimum_required(VERSION 3.12)
project(test_server)

SET("BIN_NAME" test_server)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O0 -pthread  -flto -O3 -DLIBUS_USE_OPENSSL -DLIBUS_USE_LIBUV")

set(BOOST_ROOT /usr/local/boost_1_68)

aux_source_directory(. MAIN_SRC)
###############################################################################################
include_directories(".")
include_directories("../include/uWebsockets")

find_package(Boost 1.68.0 COMPONENTS date_time filesystem system thread serialization math_tr1 chrono regex program_options unit_test_framework locale REQUIRED)
include_directories(${BOOST_INCLUDEDIR})


link_directories("/usr/local/boost_1_68/lib")

ADD_EXECUTABLE(${BIN_NAME} ${MAIN_SRC})

target_link_libraries(${BIN_NAME} crypto  uv ssl z uSockets ${Boost_LIBRARIES})

