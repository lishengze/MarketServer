cmake_minimum_required(VERSION 3.12)
project(demo4hub_demo)


###############################################################################################
include_directories("../")
include_directories("../lib")
include_directories("../base/cpp")

include_directories("../component/")

link_directories("../lib")
# for search lib quark
link_directories("/opt/utrade/console/lib/quark")
# for search lib pandora
link_directories("/opt/utrade/console/lib/pandora")

aux_source_directory(. DIR_SRCS)

###############################################################################################
add_executable(demo4hub_demo ${DIR_SRCS})
target_link_libraries(demo4hub_demo demo4hub)
TARGET_LINK_LIBRARIES(demo4hub_demo quark pandora )

#target_link_libraries(demo4hub_demo protobuf protoc protobuf-lite)
#target_link_libraries(demo4hub_demo grpc++_reflection grpc++ gpr ssl crypto)
###############################################################################################