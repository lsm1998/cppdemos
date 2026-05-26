add_executable(field_info_demo field_info.cpp)

add_executable(reflection_demo)

target_sources(reflection_demo
    PRIVATE
        main.cpp
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
            reflect.ixx
            user.ixx
            user_reflect.ixx
)

message("加载完毕")