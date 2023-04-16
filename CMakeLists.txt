cmake_minimum_required(VERSION 3.0)

set (car_app_name carctrl)
set (cli_app_name interface)
project(${car_app_name})

set(CMAKE_CXX_FLAGS "-std=c++20")
SET(CMAKE_BUILD_TYPE "Debug")

find_package(FMT REQUIRED)

add_executable(${car_app_name} mcarctrl)
target_link_libraries (${car_app_name} ioapi mcarctrl)

add_executable(${cli_app_name} cli)
target_link_libraries(${cli_app_name} cli ioapi video)

add_subdirectory (ioapi)
add_subdirectory (mcarctrl)
add_subdirectory (cli)
add_subdirectory (video)
add_subdirectory (tools)
