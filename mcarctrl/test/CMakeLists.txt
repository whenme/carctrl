# Objects already produced by the mcarctrl library. Do not compile mcarctrl/src again.
set (mcarctrl_obj_dir ${CMAKE_BINARY_DIR}/mcarctrl/CMakeFiles/mcarctrl.dir/src)

# main.cpp.o defines main. Rename that symbol so this executable can keep its
# own main and still call the program entry as agvctrl_main.
set (mcarctrl_main_obj ${mcarctrl_obj_dir}/main.cpp.o)
set (mcarctrl_main_renamed ${CMAKE_CURRENT_BINARY_DIR}/agvctrl_main.cpp.o)
add_custom_command (
    OUTPUT ${mcarctrl_main_renamed}
    COMMAND ${CMAKE_OBJCOPY} --redefine-sym main=agvctrl_main
            ${mcarctrl_main_obj} ${mcarctrl_main_renamed}
    DEPENDS mcarctrl ${mcarctrl_main_obj}
    COMMENT "Renaming mcarctrl main symbol for unit tests"
    VERBATIM)

add_library (mcarctrl_gpio_fake SHARED gpio_fake.cpp)
target_include_directories (mcarctrl_gpio_fake PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries (mcarctrl_gpio_fake PRIVATE dl)
target_compile_options (mcarctrl_gpio_fake PRIVATE -O0 -g -fPIC)

set (mcarctrl_objects
    ${mcarctrl_obj_dir}/car_ctrl.cpp.o
    ${mcarctrl_obj_dir}/car_speed.cpp.o
    ${mcarctrl_obj_dir}/gpio.cpp.o
    ${mcarctrl_obj_dir}/motor.cpp.o
    ${mcarctrl_obj_dir}/rpc_service.cpp.o
    ${mcarctrl_obj_dir}/steer.cpp.o)
set_source_files_properties (${mcarctrl_objects} PROPERTIES
    EXTERNAL_OBJECT TRUE
    GENERATED TRUE)

add_executable (mcarctrl_test test_mcarctrl.cpp ${mcarctrl_main_renamed} ${mcarctrl_objects})
target_include_directories (mcarctrl_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${PROJECT_SOURCE_DIR}/include)
target_compile_definitions (mcarctrl_test PRIVATE
    MCARCTRL_PARAM_JSON="${CMAKE_CURRENT_SOURCE_DIR}/../param.json")
target_compile_options (mcarctrl_test PRIVATE -O0 -g)
target_link_libraries (mcarctrl_test PRIVATE xapi fmt spdlog pthread dl)
target_link_options (mcarctrl_test PRIVATE --coverage)
add_dependencies (mcarctrl_test mcarctrl mcarctrl_gpio_fake)

enable_testing ()
add_test (NAME mcarctrl_test COMMAND mcarctrl_test)
set_tests_properties (mcarctrl_test PROPERTIES
    ENVIRONMENT "LD_PRELOAD=$<TARGET_FILE:mcarctrl_gpio_fake>")

# `make test` compiles and runs the tests. `make` does not.
add_custom_target (test
    COMMAND $<TARGET_FILE:mcarctrl_test>
    DEPENDS mcarctrl_test
    COMMENT "Running mcarctrl unit tests"
    USES_TERMINAL
)
