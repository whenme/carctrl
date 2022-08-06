cmake_minimum_required(VERSION 2.6)

set (execute_name carctrl)
project(${execute_name})
set(CMAKE_CXX_FLAGS "-std=c++20")

add_executable(${execute_name} mcarctrl)
target_link_libraries (${execute_name} ioapi mcarctrl cli)

add_subdirectory (ioapi)
add_subdirectory (mcarctrl)
add_subdirectory (cli)
