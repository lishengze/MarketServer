CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(demo4hub_demo)

###############################################################################################
INCLUDE_DIRECTORIES("../lib")
LINK_DIRECTORIES("../lib")
AUX_SOURCE_DIRECTORY(. DIR_SRCS)

###############################################################################################
add_executable(demo4hub_demo ${DIR_SRCS})
target_link_libraries(demo4hub_demo demo4hub)
###############################################################################################