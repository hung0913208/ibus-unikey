include_directories(${CMAKE_SOURCE_DIR}/Base/Libraries
		    ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cc")

add_library(ukengine STATIC ${SOURCES})

