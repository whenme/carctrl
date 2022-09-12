
set (library_name cli)
add_library (${library_name} src/cli_example.cpp
                             src/cli_impl.cpp)

target_include_directories (${library_name} PUBLIC include)

target_link_libraries (${library_name} ioapi mcarctrl)
