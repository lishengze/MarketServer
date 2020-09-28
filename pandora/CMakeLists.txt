# copyright [2018] [wxblockchain]
# author: daniel.bian
# description: pandora library main CMakeLists.txt
# update-date:
#   2019-01-04  init version
############################################################################################################################
CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(pandora)
SET(CMAKE_CXX_STANDARD 11)

FIND_PACKAGE(Boost)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

############################################################################################################################
INCLUDE_DIRECTORIES(".")
INCLUDE_DIRECTORIES("..")
# for include assign.h
INCLUDE_DIRECTORIES("../quark/cxx")
INCLUDE_DIRECTORIES("../quark/cxx/stg")
INCLUDE_DIRECTORIES("../asio_httpclient/src")
# link directory
LINK_DIRECTORIES("../quark/cxx")
LINK_DIRECTORIES("../asio_httpclient")

############################################################################################################################
# set the src file to compile
aux_source_directory(messager MESSAGER_SRC)
aux_source_directory(order ORDER_SRC)
aux_source_directory(package PACKAGE_SRC)
aux_source_directory(position POSITION_SRC)
aux_source_directory(redis REDIS_SRC)
aux_source_directory(util UTIL_SRC)
aux_source_directory(market MARKET_SRC)
# set source list
SET(TOOL_SRC ${MESSAGER_SRC} ${ORDER_SRC} ${PACKAGE_SRC} ${POSITION_SRC} ${REDIS_SRC} ${UTIL_SRC} ${MARKET_SRC})

############################################################################################################################
# generate the library
ADD_LIBRARY(${PROJECT_NAME} SHARED ${TOOL_SRC})
# 如果要生成同名的需要调用　SET_TARGET_PROPERTIES
# ADD_LIBRARY(${PROJECT_NAME}_static STATIC ${TOOL_SRC})

# SET_TARGET_PROPERTIES(${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
# 这里可以尝试设定版本, VERSION指动态库版本，SOVERSION指API版本
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES VERSION 1.0 SOVERSION 1)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -Wno-error=deprecated-declarations -Wno-deprecated-declarations")

# link external lib
TARGET_LINK_LIBRARIES(${PROJECT_NAME} log4cplus ssl curl hiredis event quark zmq asio_httpclient)

############################################################################################################################
# installation
# install directory to the right path
INSTALL(DIRECTORY messager order position redis util package DESTINATION "include/${PROJECT_NAME}" 
                                                             PATTERN "*.cpp" EXCLUDE 
                                                             PATTERN "package.h" EXCLUDE 
                                                             PATTERN "channel.h" EXCLUDE)
# install special files to include
INSTALL(FILES package/package.h package/channel.h envdeclare.h DESTINATION "include")
# install pandora_declare.h
INSTALL(FILES pandora_declare.h DESTINATION "include/${PROJECT_NAME}")

# INSTALL(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_static 
#         ARCHIVE DESTINATION "lib/${PROJECT_NAME}"
#         LIBRARY DESTINATION "lib/${PROJECT_NAME}")
# install library
INSTALL(TARGETS ${PROJECT_NAME} 
        ARCHIVE DESTINATION "lib/${PROJECT_NAME}"
        LIBRARY DESTINATION "lib/${PROJECT_NAME}")

############################################################################################################################
