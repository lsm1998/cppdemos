#include <iostream>
#include <lua.hpp>

// 检查lua返回值是否OK
static bool check_lua(lua_State* L, int r)
{
    if (r != LUA_OK)
    {
        std::cerr << "Lua error: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
        return false;
    }
    return true;
}

// 通过字符串运行lua脚本
static void demo_run_string(lua_State* L)
{
    std::cout << "=== Demo 1: luaL_dostring ===" << std::endl;
    const char* code = R"(
        print("Hello from Lua!")
        x = 1 + 2 * 3
        name = "Lua"
    )";
    check_lua(L, luaL_dostring(L, code));
    
    lua_getglobal(L, "x");
    std::cout << "  x = " << lua_tonumber(L, -1) << std::endl;
    lua_pop(L, 1);

    lua_getglobal(L, "name");
    std::cout << "  name = " << lua_tostring(L, -1) << std::endl;
    lua_pop(L, 1);
    std::cout << std::endl;
}

// 求和
static int cpp_add(lua_State* L)
{
    int nargs = lua_gettop(L);
    double sum = 0;
    for (int i = 1; i <= nargs; ++i)
        sum += luaL_checknumber(L, i);
    lua_pushnumber(L, sum);
    return 1;
}

// 返回一个table
static int cpp_make_config(lua_State* L)
{
    lua_newtable(L);
    lua_pushstring(L, "version");
    lua_pushnumber(L, 1.0);
    lua_settable(L, -3);
    lua_pushstring(L, "debug");
    lua_pushboolean(L, true);
    lua_settable(L, -3);
    lua_pushstring(L, "author");
    lua_pushstring(L, "lsm");
    lua_settable(L, -3);
    return 1;
}

// 遍历并打印一个table
static int cpp_print_table(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    std::cout << "  [cpp_print_table] {" << std::endl;
    lua_pushnil(L);
    while (lua_next(L, 1))
    {
        // key 在 -2，value 在 -1
        std::cout << "    ";
        if (lua_type(L, -2) == LUA_TSTRING)
            std::cout << lua_tostring(L, -2);
        else
            std::cout << lua_tonumber(L, -2);
        std::cout << " = ";
        if (lua_type(L, -1) == LUA_TSTRING)
            std::cout << "\"" << lua_tostring(L, -1) << "\"";
        else if (lua_type(L, -1) == LUA_TNUMBER)
            std::cout << lua_tonumber(L, -1);
        else if (lua_type(L, -1) == LUA_TBOOLEAN)
            std::cout << (lua_toboolean(L, -1) ? "true" : "false");
        std::cout << std::endl;
        lua_pop(L, 1);
    }
    std::cout << "  }" << std::endl;
    return 0;
}

// 注册C函数供Lua调用
static void demo_c_functions(lua_State* L)
{
    std::cout << "=== Demo 2: Register & call C functions from Lua ===" << std::endl;

    lua_register(L, "cpp_add", cpp_add);
    lua_register(L, "cpp_make_config", cpp_make_config);
    lua_register(L, "cpp_print_table", cpp_print_table);

    check_lua(L, luaL_dostring(L, R"(
        print("  cpp_add(10, 20, 30) = " .. cpp_add(10, 20, 30))

        local cfg = cpp_make_config()
        print("  cpp_make_config returned a table:")
        cpp_print_table(cfg)
    )"));
    std::cout << std::endl;
}

// 调用Lua函数
static void demo_call_lua(lua_State* L)
{
    std::cout << "=== Demo 3: Call Lua function from C++ ===" << std::endl;

    // 定义Lua函数
    check_lua(L, luaL_dostring(L, R"(
        function multiply(a, b)
            return a * b
        end
        function greet(name, times)
            local t = {}
            for i = 1, times do
                t[i] = "Hello, " .. name .. "!"
            end
            return t
        end
    )"));

    // --- 调用 multiply(6, 7) ---
    lua_getglobal(L, "multiply");
    lua_pushnumber(L, 6);
    lua_pushnumber(L, 7);
    check_lua(L, lua_pcall(L, 2, 1, 0));
    std::cout << "  multiply(6, 7) = " << lua_tonumber(L, -1) << std::endl;
    lua_pop(L, 1);

    // --- 调用 greet("World", 3)，返回一个 table ---
    lua_getglobal(L, "greet");
    lua_pushstring(L, "World");
    lua_pushinteger(L, 3);
    check_lua(L, lua_pcall(L, 2, 1, 0));

    std::cout << "  greet(\"World\", 3) = [" << std::endl;
    lua_pushnil(L);
    while (lua_next(L, -2))
    {
        std::cout << "    " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
    }
    std::cout << "  ]" << std::endl;
    lua_pop(L, 1);
    std::cout << std::endl;
}

// 读写Lua全局变量
static void demo_globals(lua_State* L)
{
    std::cout << "=== Demo 4: Set / get globals ===" << std::endl;

    lua_pushnumber(L, 42);
    lua_setglobal(L, "the_answer");

    lua_pushstring(L, "Hello from C++");
    lua_setglobal(L, "greeting");

    check_lua(L, luaL_dostring(L, R"(
        print("  the_answer = " .. the_answer)
        print("  greeting = " .. greeting)
    )"));
    std::cout << std::endl;
}

// 操作Lua table
static void demo_tables(lua_State* L)
{
    std::cout << "=== Demo 5: Table manipulation from C++ ===" << std::endl;

    // 构造一个类数组的 table {1, 4, 9, 16, 25}
    lua_newtable(L);
    for (int i = 1; i <= 5; ++i)
    {
        lua_pushinteger(L, i);
        lua_pushinteger(L, i * i);
        lua_settable(L, -3);
    }
    lua_setglobal(L, "squares");

    // 构造一个嵌套 table：{name="Alice", scores={95, 88, 97}}
    lua_newtable(L);
    lua_pushstring(L, "name");
    lua_pushstring(L, "Alice");
    lua_settable(L, -3);

    lua_pushstring(L, "scores");
    lua_newtable(L);
    for (int i = 1; i <= 3; ++i)
    {
        lua_pushinteger(L, i);
        lua_pushinteger(L, 90 + i); // 仅为演示用的值
        lua_settable(L, -3);
    }
    lua_settable(L, -3);

    lua_setglobal(L, "student");

    // 从 Lua 端读回验证
    check_lua(L, luaL_dostring(L, R"(
        print("  squares[3] = " .. squares[3])
        print("  squares[5] = " .. squares[5])
        print("  student:")
        for k, v in pairs(student) do
            if type(v) == "table" then
                print("    " .. k .. " = {")
                for i, s in ipairs(v) do
                    print("      " .. i .. " = " .. s)
                end
                print("    }")
            else
                print("    " .. k .. " = " .. v)
            end
        end
    )"));
    std::cout << std::endl;
}

// 错误处理
static void demo_error_handling(lua_State* L)
{
    std::cout << "=== Demo 6: Error handling ===" << std::endl;

    // dostring 捕获语法错误
    int r = luaL_dostring(L, "print(1 + ");
    if (r != LUA_OK)
    {
        std::cout << "  Caught error (dostring): " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
    }

    check_lua(L, luaL_dostring(L, "function oops() error('boom!') end"));
    lua_getglobal(L, "oops");
    r = lua_pcall(L, 0, 0, 0);
    if (r != LUA_OK)
    {
        std::cout << "  Caught error (pcall):   " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
    }

    std::cout << std::endl;
}

// 加载并运行外部Lua文件
static void demo_load_file(lua_State* L)
{
    std::cout << "=== Demo 7: Load & run test1.lua ===" << std::endl;

    int r = luaL_dofile(L, "test1.lua");
    if (r != LUA_OK)
    {
        std::cout << "  File error: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
        std::cout << std::endl;
        return;
    }

    // 调用 test1.lua 中定义的 fibonacci()
    lua_getglobal(L, "fibonacci");
    if (lua_isfunction(L, -1))
    {
        lua_pushinteger(L, 10);
        check_lua(L, lua_pcall(L, 1, 1, 0));
        std::cout << "  fibonacci(10) = " << lua_tonumber(L, -1) << std::endl;
        lua_pop(L, 1);
    }
    else
    {
        lua_pop(L, 1);
    }

    // 读取 test1.lua 中的 demo_config table
    lua_getglobal(L, "demo_config");
    if (lua_istable(L, -1))
    {
        std::cout << "  demo_config:" << std::endl;
        lua_pushnil(L);
        while (lua_next(L, -2))
        {
            std::cout << "    ";
            if (lua_type(L, -2) == LUA_TSTRING)
                std::cout << lua_tostring(L, -2);
            std::cout << " = ";
            if (lua_type(L, -1) == LUA_TSTRING)
                std::cout << "\"" << lua_tostring(L, -1) << "\"";
            else if (lua_type(L, -1) == LUA_TNUMBER)
                std::cout << lua_tonumber(L, -1);
            else if (lua_type(L, -1) == LUA_TBOOLEAN)
                std::cout << (lua_toboolean(L, -1) ? "true" : "false");
            else if (lua_type(L, -1) == LUA_TTABLE)
                std::cout << "{...}";
            std::cout << std::endl;
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    std::cout << std::endl;
}

int main()
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    std::cout << "Lua version: " << LUA_VERSION << std::endl << std::endl;

    demo_run_string(L);
    demo_c_functions(L);
    demo_call_lua(L);
    demo_globals(L);
    demo_tables(L);
    demo_error_handling(L);
    demo_load_file(L);

    lua_close(L);
    std::cout << "Done." << std::endl;
    return 0;
}
