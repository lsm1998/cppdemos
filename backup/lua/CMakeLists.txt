find_package(Lua REQUIRED)
# sudo apt install liblua5.4-dev
include_directories(${LUA_INCLUDE_DIR})

add_executable(lua main.cpp)

target_link_libraries(lua ${LUA_LIBRARIES})