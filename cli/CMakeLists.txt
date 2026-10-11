
find_package(OpenCV 4.5 REQUIRED)

set (library_name cli)
add_library (${library_name} src/cli_example.cpp
                             src/cli_impl.cpp
                             src/cli_car.cpp
                             src/rpc_client.cpp
                             src/car_web_server.cpp
                             src/cli_video.cpp
                             src/remote_key.cpp
                             src/main.cpp)

target_include_directories (${library_name} PUBLIC include ${PROJECT_SOURCE_DIR}/include
                                                   ${OpenCV_INCLUDE_DIRS})

target_compile_definitions(${library_name} PRIVATE ASYNC_SIMPLE_HAS_NOT_AIO)

target_link_libraries (${library_name} xapi video fmt ${OpenCV_LIBS})

# coro_rpc headers need about 1GB to compile. Ask GCC to collect garbage
# more often so armhf boards are less likely to kill cc1plus.
set_source_files_properties(src/rpc_client.cpp src/car_web_server.cpp src/main.cpp src/remote_key.cpp
    PROPERTIES COMPILE_OPTIONS "--param=ggc-min-expand=10;--param=ggc-min-heapsize=32768")
