
add_executable(calamares calamares_bin.cpp)
set_property(TARGET calamares PROPERTY CXX_STANDARD 20)
set_property(TARGET calamares PROPERTY CXX_STANDARD_REQUIRED ON)

install (PROGRAMS execute.sh DESTINATION ${PROJECT_BINARY_DIR})
install (PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/calamares DESTINATION ${PROJECT_BINARY_DIR})
