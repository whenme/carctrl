
set (library_name xapi)

add_library (${library_name} src/param_json.cpp
                             src/iotimer.cpp
                             src/iosignal.cpp
                             src/cmn_thread.cpp
                             src/pty_shell.cpp)
                             #src/cmn_assert.cpp)

target_include_directories (${library_name} PUBLIC include ../include)
