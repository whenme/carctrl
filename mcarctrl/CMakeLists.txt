
set (library_name mcarctrl)

set (param_file car.json)

add_library (${library_name} src/car_ctrl.cpp
                             src/car_speed.cpp
                             src/gpio.cpp
                             src/remote_key.cpp
                             src/cli_car.cpp
                             src/main.cpp)

target_include_directories (${library_name} PUBLIC include)

target_link_libraries (${library_name} ioapi cli video)

install (FILES ${param_file} DESTINATION ${PROJECT_BINARY_DIR})
