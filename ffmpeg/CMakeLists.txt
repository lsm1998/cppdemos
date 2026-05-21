# sudo apt install ffmpeg libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libavdevice-dev libavfilter-dev libswresample-dev

cmake_minimum_required(VERSION 3.12)
project(ffmpeg-demos)

find_package(PkgConfig REQUIRED)

pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
    libavdevice
    libavfilter
    libswresample
)

file(GLOB DEMO_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(src ${DEMO_SOURCES})
    get_filename_component(name ${src} NAME_WE)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE PkgConfig::FFMPEG)
endforeach()
