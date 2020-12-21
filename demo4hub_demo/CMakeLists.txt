CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(demo4hub_demo)

###############################################################################################
INCLUDE_DIRECTORIES("../")
INCLUDE_DIRECTORIES("../lib")
INCLUDE_DIRECTORIES("../base/cpp")

INCLUDE_DIRECTORIES("../../component/")

LINK_DIRECTORIES("../lib")
# for search lib quark
LINK_DIRECTORIES("/opt/utrade/console/lib/quark")
# for search lib pandora
LINK_DIRECTORIES("/opt/utrade/console/lib/pandora")

AUX_SOURCE_DIRECTORY(. DIR_SRCS)

###############################################################################################
add_executable(demo4hub_demo ${DIR_SRCS})
target_link_libraries(demo4hub_demo demo4hub)
TARGET_LINK_LIBRARIES(demo4hub_demo quark pandora )

#target_link_libraries(demo4hub_demo protobuf protoc protobuf-lite)
#target_link_libraries(demo4hub_demo grpc++_reflection grpc++ gpr ssl crypto)
###############################################################################################