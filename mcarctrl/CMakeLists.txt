set (library_name mcarctrl)

set (param_file param.json)

add_library (${library_name} src/car_ctrl.cpp
                             src/car_speed.cpp
                             src/gpio.cpp
                             src/motor.cpp
                             src/main.cpp
                             src/rpc_service.cpp
                             src/steer.cpp)

target_include_directories (${library_name} PUBLIC include ${PROJECT_SOURCE_DIR}/include)

target_link_libraries (${library_name} xapi fmt)

install (FILES ${param_file} DESTINATION ${PROJECT_BINARY_DIR})
