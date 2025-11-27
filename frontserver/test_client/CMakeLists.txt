# copyright [2018] [wxblockchain]
# author: daniel.bian
# description: galaxy project main CMakeLists.txt
# update-date:
#   2019-01-04  init version
##############################################################################################
cmake_minimum_required(VERSION 3.12)
project(test_client)

SET("BIN_NAME" test_client)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O0 -pthread  -flto -O3 -DLIBUS_USE_OPENSSL -DLIBUS_USE_LIBUV")

set(BOOST_ROOT /usr/local/boost_1_68)

AUX_SOURCE_DIRECTORY(. MAIN_SRC)

###############################################################################################
include_directories(".")
include_directories("./uWebsockets")
# for include package.h and channel.h
include_directories("../../../component/pandora/package")
# for include assign.h
include_directories("../../../component/quark/cxx")
# for include envdeclare.h
include_directories("../../../component/pandora")

include_directories("../../../component/quark/cxx/stg")
# for include envdeclare.h
include_directories("../../../component/")
include_directories("../../../component/")
include_directories("../../../component/pandora/util")
include_directories("../../../component/pandora/messager")
include_directories("../../../component/asio_httpclient")
include_directories("../../../component/asio_httpclient/src")
include_directories("../../../component/comm")
include_directories("../../../component/comm/stg_trade")

find_package(Boost 1.68.0 COMPONENTS date_time filesystem system thread serialization math_tr1 chrono regex program_options unit_test_framework locale REQUIRED)
include_directories(${BOOST_INCLUDEDIR})

# for search lib quark
link_directories("/opt/utrade/console/lib/quark")
# for search lib pandora
link_directories("/opt/utrade/console/lib/pandora")

link_directories("/opt/utrade/console/lib/comm")

link_directories("/opt/utrade/console/lib/asio_httpclient")

link_directories("/usr/local/boost_1_68/lib")

ADD_EXECUTABLE(${BIN_NAME} ${MAIN_SRC})

TARGET_LINK_LIBRARIES(${BIN_NAME} asio_httpclient quark pandora log4cplus crypto comm uv ssl z uSockets ${Boost_LIBRARIES})

