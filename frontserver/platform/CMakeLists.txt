# copyright [2018] [wxblockchain]
# author: daniel.bian
# description: galaxy project main CMakeLists.txt
# update-date:
#   2019-01-04  init version
##############################################################################################
CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(FrontServer)

SET("BIN_NAME" front_server)
set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O0 -pthread  -flto -O3 -DLIBUS_USE_OPENSSL -DLIBUS_USE_LIBUV")

set(BOOST_ROOT /usr/local/boost_1_68)

add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
#add_dependencies(-DBOOST_SYSTEM_NO_DEPRECATED)

AUX_SOURCE_DIRECTORY(. MAIN_SRC)
AUX_SOURCE_DIRECTORY(./data_process DATA_PROCESS_SRC)
AUX_SOURCE_DIRECTORY(./data_receive DATA_RECEIVE_SRC)
AUX_SOURCE_DIRECTORY(./front_server FRONT_SERVER_SRC)
AUX_SOURCE_DIRECTORY(./util UTIL_SRC)
AUX_SOURCE_DIRECTORY(./config CONFIG_SRC)
AUX_SOURCE_DIRECTORY(./log LOG_SRC)

###############################################################################################
INCLUDE_DIRECTORIES(".")
INCLUDE_DIRECTORIES("../include")
INCLUDE_DIRECTORIES("../include/uWebsockets")
# for include package.h and channel.h
INCLUDE_DIRECTORIES("../../component/pandora/package")
# for include assign.h
INCLUDE_DIRECTORIES("../../component/quark/cxx")
# for include envdeclare.h
INCLUDE_DIRECTORIES("../../component/pandora")

INCLUDE_DIRECTORIES("../../component/quark/cxx/stg")
# for include envdeclare.h
INCLUDE_DIRECTORIES("../../component/")
INCLUDE_DIRECTORIES("../../component/")
INCLUDE_DIRECTORIES("../../component/pandora/util")
INCLUDE_DIRECTORIES("../../component/pandora/messager")
INCLUDE_DIRECTORIES("../../component/asio_httpclient")
INCLUDE_DIRECTORIES("../../component/asio_httpclient/src")
INCLUDE_DIRECTORIES("../../component/comm")
INCLUDE_DIRECTORIES("../../component/comm/stg_trade")

FIND_PACKAGE(Boost 1.68.0 COMPONENTS date_time filesystem system thread serialization math_tr1 chrono regex program_options unit_test_framework locale REQUIRED)
INCLUDE_DIRECTORIES(${BOOST_INCLUDEDIR})

# for search lib quark
LINK_DIRECTORIES("/opt/utrade/console/lib/quark")
# for search lib pandora
LINK_DIRECTORIES("/opt/utrade/console/lib/pandora")

LINK_DIRECTORIES("/opt/utrade/console/lib/comm")

LINK_DIRECTORIES("/opt/utrade/console/lib/asio_httpclient")

LINK_DIRECTORIES("/usr/local/boost_1_68/lib")

LINK_DIRECTORIES("../lib")

ADD_EXECUTABLE(${BIN_NAME} ${MAIN_SRC} ${DATA_PROCESS_SRC} 
               ${DATA_RECEIVE_SRC} ${FRONT_SERVER_SRC} 
               ${UTIL_SRC} ${CONFIG_SRC} ${LOG_SRC})

TARGET_LINK_LIBRARIES(${BIN_NAME} ${Boost_LIBRARIES})
TARGET_LINK_LIBRARIES(${BIN_NAME} ssl crypto uv z uSockets )
TARGET_LINK_LIBRARIES(${BIN_NAME} demo4hub)
TARGET_LINK_LIBRARIES(${BIN_NAME} asio_httpclient quark pandora comm log4cplus)

