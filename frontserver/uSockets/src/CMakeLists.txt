# copyright [2018] [wxblockchain]
# author: daniel.bian
# description: pandora library main CMakeLists.txt
# update-date:
#   2019-01-04  init version
############################################################################################################################
CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(uSockets)
SET(CMAKE_CXX_STANDARD 11)

############################################################################################################################
INCLUDE_DIRECTORIES(".")
INCLUDE_DIRECTORIES("..")


############################################################################################################################
# set the src file to compile
aux_source_directory(. MAIN_SRC)
aux_source_directory(eventing EVENT_SRC)
aux_source_directory(crypto CRYPTO_SRC)
aux_source_directory(internal INTERNAL_SRC)

# set source list
SET(TOOL_SRC ${MAIN_SRC}  ${EVENT_SRC} ${CRYPTO_SRC} ${INTERNAL_SRC})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLIBUS_USE_OPENSSL -DLIBUS_USE_LIBUV")

############################################################################################################################
# generate the library
ADD_LIBRARY(${PROJECT_NAME} SHARED ${TOOL_SRC})
# 如果要生成同名的需要调用　SET_TARGET_PROPERTIES
# ADD_LIBRARY(${PROJECT_NAME}_static STATIC ${TOOL_SRC})

# link external lib
TARGET_LINK_LIBRARIES(${PROJECT_NAME} uv ssl crypto z)

#TARGET_LINK_LIBRARIES(${PROJECT_NAME} log4cplus ssl curl hiredis event quark zmq asio_httpclient SQLiteCpp sqlite3 dl)

# ############################################################################################################################
# # installation
# # install directory to the right path
# INSTALL(DIRECTORY messager order position redis util package DESTINATION "include/${PROJECT_NAME}" 
#                                                              PATTERN "*.cpp" EXCLUDE 
#                                                              PATTERN "package.h" EXCLUDE 
#                                                              PATTERN "channel.h" EXCLUDE)
# # install special files to include
# INSTALL(FILES package/package.h package/channel.h envdeclare.h DESTINATION "include")
# # install pandora_declare.h
# INSTALL(FILES pandora_declare.h DESTINATION "include/${PROJECT_NAME}")

# # INSTALL(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_static 
# #         ARCHIVE DESTINATION "lib/${PROJECT_NAME}"
# #         LIBRARY DESTINATION "lib/${PROJECT_NAME}")
# # install library
# INSTALL(TARGETS ${PROJECT_NAME} 
#         ARCHIVE DESTINATION "lib/${PROJECT_NAME}"
#         LIBRARY DESTINATION "lib/${PROJECT_NAME}")

############################################################################################################################
