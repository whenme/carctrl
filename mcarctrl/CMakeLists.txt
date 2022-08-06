
set (library_name mcarctrl)

add_library (${library_name} CarCtrl.cpp
                             CarSpeed.cpp
                             Gpio.cpp
                             RemoteKey.cpp
                             main.cpp)

target_include_directories (${library_name} PUBLIC include)
