cmake_minimum_required(VERSION 2.6)

project(carctrl)

set(CMAKE_CXX_FLAGS "-std=c++11")

add_executable(carctrl main.cpp CarCtrl.cpp CarSpeed.cpp RemoteKey.cpp Gpio.cpp)


