# copyright [2018] [wxblockchain]
# author: daniel.bian
# description: galaxy project main CMakeLists.txt
# update-date:
#   2019-01-04  init version
##############################################################################################
cmake_minimum_required(VERSION 3.12)
project(FrontServer)

SET("BIN_NAME" front_server)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O0 -pthread -g -flto -O3 -DLIBUS_USE_OPENSSL -DLIBUS_USE_LIBUV")

set(BOOST_ROOT /usr/local/boost_1_68)

add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
#add_dependencies(-DBOOST_SYSTEM_NO_DEPRECATED)

aux_source_directory(. MAIN_SRC)
aux_source_directory(./data_process DATA_PROCESS_SRC)
aux_source_directory(./data_receive DATA_RECEIVE_SRC)
aux_source_directory(./data_structure DATA_STRUCTURE_SRC)
aux_source_directory(./front_server FRONT_SERVER_SRC)
aux_source_directory(./util UTIL_SRC)
aux_source_directory(./config CONFIG_SRC)
aux_source_directory(./log LOG_SRC)

###############################################################################################
include_directories(".")
include_directories("../../lib")
include_directories("../../base/cpp")
include_directories("../../")

include_directories("../include/uWebsockets")
# for include package.h and channel.h
include_directories("../../component/pandora/package")
# for include assign.h
include_directories("../../component/quark/cxx")
# for include envdeclare.h
include_directories("../../component/pandora")

include_directories("../../component/quark/cxx/stg")
# for include envdeclare.h
include_directories("../../component/")
include_directories("../../component/")
include_directories("../../component/pandora/util")
include_directories("../../component/pandora/messager")
include_directories("../../component/asio_httpclient")
include_directories("../../component/asio_httpclient/src")
include_directories("../../component/comm")
include_directories("../../component/comm/stg_trade")

find_package(Boost 1.68.0 COMPONENTS date_time filesystem system thread serialization math_tr1 chrono regex program_options unit_test_framework locale REQUIRED)
include_directories(${BOOST_INCLUDEDIR})

# for search lib quark
link_directories("/opt/utrade/console/lib/quark")
# for search lib pandora
link_directories("/opt/utrade/console/lib/pandora")

link_directories("/opt/utrade/console/lib/comm")

link_directories("/opt/utrade/console/lib/asio_httpclient")

link_directories("/usr/local/boost_1_68/lib")

# link_directories("../lib")

link_directories("../../lib")

ADD_EXECUTABLE(${BIN_NAME} ${MAIN_SRC} ${DATA_STRUCTURE_SRC} ${DATA_PROCESS_SRC} 
               ${DATA_RECEIVE_SRC} ${FRONT_SERVER_SRC} 
               ${UTIL_SRC} ${CONFIG_SRC} ${LOG_SRC})

target_link_libraries(${BIN_NAME} ${Boost_LIBRARIES})
target_link_libraries(${BIN_NAME} ssl crypto uv z uSockets )
target_link_libraries(${BIN_NAME} demo4hub)
target_link_libraries(${BIN_NAME} asio_httpclient quark pandora comm log4cplus)

