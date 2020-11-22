#####################################################################################################################
CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(platform)

#####################################################################################################################
SET("BIN_NAME" frontserver)

AUX_SOURCE_DIRECTORY(. MAIN_SRC)

#####################################################################################################################
ADD_EXECUTABLE(${BIN_NAME} ${MAIN_SRC})

TARGET_LINK_LIBRARIES(${BIN_NAME} quark pandora log4cplus crypto comm uv ssl z uSockets ${Boost_LIBRARIES})

#####################################################################################################################
INSTALL(TARGETS ${BIN_NAME} RUNTIME DESTINATION "bin")
