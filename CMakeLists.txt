cmake_minimum_required(VERSION 3.0)

set (car_app_name agvctrl)
set (cli_app_name agvintf)
project(${car_app_name})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "-std=c++23")
SET(CMAKE_BUILD_TYPE "Debug")

find_package(FMT REQUIRED)

add_executable(${car_app_name} mcarctrl)
target_link_libraries (${car_app_name} xapi mcarctrl)

add_executable(${cli_app_name} cli)
target_link_libraries(${cli_app_name} cli xapi video)

add_subdirectory (xapi)
add_subdirectory (mcarctrl)
add_subdirectory (cli)
add_subdirectory (video)
add_subdirectory (tools)
