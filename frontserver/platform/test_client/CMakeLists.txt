# copyright [2018] [wxblockchain]
# author: daniel.bian
# description: galaxy project main CMakeLists.txt
# update-date:
#   2019-01-04  init version
##############################################################################################
CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(test_client)

SET("BIN_NAME" test_client)
set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O0 -pthread  -flto -O3 -DLIBUS_USE_OPENSSL -DLIBUS_USE_LIBUV")

set(BOOST_ROOT /usr/local/boost_1_68)

AUX_SOURCE_DIRECTORY(. MAIN_SRC)

###############################################################################################
INCLUDE_DIRECTORIES(".")
INCLUDE_DIRECTORIES("./uWebsockets")
# for include package.h and channel.h
INCLUDE_DIRECTORIES("../../../component/pandora/package")
# for include assign.h
INCLUDE_DIRECTORIES("../../../component/quark/cxx")
# for include envdeclare.h
INCLUDE_DIRECTORIES("../../../component/pandora")

INCLUDE_DIRECTORIES("../../../component/quark/cxx/stg")
# for include envdeclare.h
INCLUDE_DIRECTORIES("../../../component/")
INCLUDE_DIRECTORIES("../../../component/")
INCLUDE_DIRECTORIES("../../../component/pandora/util")
INCLUDE_DIRECTORIES("../../../component/pandora/messager")
INCLUDE_DIRECTORIES("../../../component/asio_httpclient")
INCLUDE_DIRECTORIES("../../../component/asio_httpclient/src")
INCLUDE_DIRECTORIES("../../../component/comm")
INCLUDE_DIRECTORIES("../../../component/comm/stg_trade")

FIND_PACKAGE(Boost 1.68.0 COMPONENTS date_time filesystem system thread serialization math_tr1 chrono regex program_options unit_test_framework locale REQUIRED)
INCLUDE_DIRECTORIES(${BOOST_INCLUDEDIR})

# for search lib quark
LINK_DIRECTORIES("/opt/utrade/console/lib/quark")
# for search lib pandora
LINK_DIRECTORIES("/opt/utrade/console/lib/pandora")

LINK_DIRECTORIES("/opt/utrade/console/lib/comm")

LINK_DIRECTORIES("/opt/utrade/console/lib/asio_httpclient")

LINK_DIRECTORIES("/usr/local/boost_1_68/lib")

ADD_EXECUTABLE(${BIN_NAME} ${MAIN_SRC})

TARGET_LINK_LIBRARIES(${BIN_NAME} asio_httpclient quark pandora log4cplus crypto comm uv ssl z uSockets ${Boost_LIBRARIES})

