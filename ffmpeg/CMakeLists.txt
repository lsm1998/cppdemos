# sudo apt install ffmpeg libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libavdevice-dev libavfilter-dev libswresample-dev

find_package(PkgConfig REQUIRED)

pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
    libavdevice      # 可选，如需采集设备
    libavfilter      # 可选，如需滤镜
    libswresample    # 可选，如需音频重采样
)

add_executable(ffmpeg-demo01 demo01.cpp)

target_link_libraries(ffmpeg-demo01 PRIVATE PkgConfig::FFMPEG)