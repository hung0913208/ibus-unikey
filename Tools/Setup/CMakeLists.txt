include_directories(${CMAKE_SOURCE_DIR}/Base/Libraries
		    ${CMAKE_SOURCE_DIR}/Libraries/Ukengine
		    ${CMAKE_SOURCE_DIR}/Libraries/Unikey
		    ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB COMMON "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cc")
file(GLOB ENGINE "${CMAKE_CURRENT_SOURCE_DIR}/Source/Engine/*.cc")
file(GLOB SETUP "${CMAKE_CURRENT_SOURCE_DIR}/Source/UI/*.cc")

add_executable(ibus-engine-unikey ${COMMON} ${ENGINE})
add_executable(ibus-setup-unikey ${COMMON} ${SETUP})

target_link_libraries(ibus-engine-unikey
    ukengine
    unikey
    ${IBUS_LIBRARIES}
    ${GTK3_LIBRARIES}
    ${GIO_LIBRARIES}
)
target_link_libraries(ibus-setup-unikey
    ukengine
    unikey
    ${GTK3_LIBRARIES}
)

target_include_directories(ibus-engine-unikey PUBLIC ${IBUS_INCLUDE_DIRS})
target_include_directories(ibus-setup-unikey PUBLIC ${GTK3_INCLUDE_DIRS})

configure_file(Data/unikey.xml.in unikey.xml)
gettext_process_po_files(vi ALL PO_FILES Data/po/vi.po)

install(TARGETS ibus-setup-unikey DESTINATION ${LIBEXECDIR})
install(TARGETS ibus-engine-unikey DESTINATION ${LIBEXECDIR})

install(FILES Data/ibus-unikey.ui DESTINATION ${PKGDATADIR}/ui)
install(FILES Data/ibus-unikey.svg DESTINATION ${PKGDATADIR}/icons)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/unikey.xml DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/ibus/component")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/vi.gmo"
    DESTINATION "${LOCALEDIR}/vi/LC_MESSAGES"
    RENAME "${PROJECT_NAME}.mo")

