# copyright [2018] [wxblockchain]
# author: daniel.bian
# description: galaxy project main CMakeLists.txt
# update-date:
#   2019-01-04  init version
##############################################################################################
CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(frontserver)

set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++17 -O0 -pthread  -flto -O3 -DLIBUS_USE_OPENSSL -DLIBUS_USE_LIBUV")

set(BOOST_ROOT /usr/local/boost_1_68)

###############################################################################################
INCLUDE_DIRECTORIES("..")
INCLUDE_DIRECTORIES(".")
INCLUDE_DIRECTORIES("./uWebsockets")
# for include package.h and channel.h
INCLUDE_DIRECTORIES("../component/pandora/package")
# for include assign.h
INCLUDE_DIRECTORIES("../component/quark/cxx")
# for include envdeclare.h
INCLUDE_DIRECTORIES("../component/pandora")

INCLUDE_DIRECTORIES("../component/quark/cxx/stg")
# for include envdeclare.h
INCLUDE_DIRECTORIES("../component/")
INCLUDE_DIRECTORIES("../component/pandora/util")
INCLUDE_DIRECTORIES("../component/pandora/messager")
INCLUDE_DIRECTORIES("../component/asio_httpclient/src")
INCLUDE_DIRECTORIES("../component/comm")
INCLUDE_DIRECTORIES("../component/comm/stg_trade")

FIND_PACKAGE(Boost 1.68.0 COMPONENTS date_time filesystem system thread serialization math_tr1 chrono regex program_options unit_test_framework locale REQUIRED)
INCLUDE_DIRECTORIES(${BOOST_INCLUDEDIR})

# for search lib quark
LINK_DIRECTORIES("../component/quark/cxx")
# for search lib pandora
LINK_DIRECTORIES("../component/pandora")

LINK_DIRECTORIES("../component/comm")

LINK_DIRECTORIES("/usr/local/boost_1_68/lib")

################################################################################################
ADD_SUBDIRECTORY(platform)

################################################################################################
INSTALL(DIRECTORY etc/ DESTINATION "etc")
INSTALL(DIRECTORY bin/ DESTINATION "bin")
