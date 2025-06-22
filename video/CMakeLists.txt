
set (library_name video)

find_package(OpenCV 4.5 REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

add_library (${library_name} src/video_device.cpp
                             src/sound_intf.cpp
                             src/cli_video.cpp
                             src/video_ctrl.cpp)

target_include_directories (${library_name} PUBLIC include)

target_link_libraries (${library_name} xapi cli
                                       ${OpenCV_LIBS})

install (FILES lena.png DESTINATION ${PROJECT_BINARY_DIR})
