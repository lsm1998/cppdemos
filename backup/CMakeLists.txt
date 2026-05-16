cmake_minimum_required(VERSION 3.16)
project(cppdemos)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(lua)
add_subdirectory(template)