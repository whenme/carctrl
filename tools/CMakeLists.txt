
add_executable(calamares_bin calamares_bin.c)

install (PROGRAMS execute.sh DESTINATION ${PROJECT_BINARY_DIR})
install (PROGRAMS calamares DESTINATION ${PROJECT_BINARY_DIR})
install (PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/calamares_bin DESTINATION ${PROJECT_BINARY_DIR})
