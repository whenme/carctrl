
add_executable(${PROJECT_BINARY_DIR}/calamares calamares_bin.c)

install (PROGRAMS start.sh DESTINATION ${PROJECT_BINARY_DIR})
