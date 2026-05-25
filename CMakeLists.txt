cmake_minimum_required(VERSION 3.16)
project(cppdemos)

include(CheckCXXSourceCompiles)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(lua)
add_subdirectory(template)
add_subdirectory(ffmpeg)

set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -std=c++2c -freflection")
check_cxx_source_compiles(
    "
    #include <meta>
    struct Person { int age; };
    constexpr auto member_count = std::meta::nonstatic_data_members_of(^^Person).size();
    int main() { return member_count == 1 ? 0 : 1; }
    "
    CPPDEMOS_HAS_STD_REFLECTION
)

if(CPPDEMOS_HAS_STD_REFLECTION)
    add_subdirectory(reflection)
else()
    message(STATUS "Skipping reflection demo: compiler does not support C++ reflection (-freflection).")
endif()
