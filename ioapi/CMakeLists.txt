
set (library_name ioapi)

add_library (${library_name} src/param_json.cpp)

target_include_directories (${library_name} PUBLIC include)