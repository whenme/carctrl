
add_executable(calamares calamares_bin.c)

install (PROGRAMS execute.sh DESTINATION ${PROJECT_BINARY_DIR})
install (PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/calamares DESTINATION ${PROJECT_BINARY_DIR})
