include_directories(${CMAKE_SOURCE_DIR}/Base/Libraries
		    ${CMAKE_SOURCE_DIR}/Libraries/Ukengine
		    ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cc")

add_library(unikey ${SOURCES})
add_schemas(unikey "${CMAKE_CURRENT_SOURCE_DIR}/Schema" "${CMAKE_INSTALL_PREFIX}")

target_link_libraries(unikey ukengine ${GIO_LIBRARIES})
target_include_directories(unikey PUBLIC ${GIO_INCLUDE_DIR})
