
set (library_name cli)
add_library (${library_name} src/cli_example.cpp)

target_include_directories (${library_name} PUBLIC include)

