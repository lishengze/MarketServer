# copyright [2018] [wxblockchain]
# author: daniel.bian
# description: galaxy project main CMakeLists.txt
# update-date:
#   2019-01-04  init version
##############################################################################################
CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(test_server)

SET("BIN_NAME" test_server)
set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O0 -pthread  -flto -O3 -DLIBUS_USE_OPENSSL -DLIBUS_USE_LIBUV")

set(BOOST_ROOT /usr/local/boost_1_68)

AUX_SOURCE_DIRECTORY(. MAIN_SRC)
###############################################################################################
INCLUDE_DIRECTORIES(".")
INCLUDE_DIRECTORIES("../uWebsockets")

FIND_PACKAGE(Boost 1.68.0 COMPONENTS date_time filesystem system thread serialization math_tr1 chrono regex program_options unit_test_framework locale REQUIRED)
INCLUDE_DIRECTORIES(${BOOST_INCLUDEDIR})


LINK_DIRECTORIES("/usr/local/boost_1_68/lib")


ADD_EXECUTABLE(${BIN_NAME} ${MAIN_SRC})

TARGET_LINK_LIBRARIES(${BIN_NAME} crypto  uv ssl z uSockets ${Boost_LIBRARIES})

