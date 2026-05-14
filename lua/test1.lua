-- 递归实现的斐波那契数列
function fibonacci(n)
    if n < 2 then
        return n
    end
    return fibonacci(n - 1) + fibonacci(n - 2)
end

-- 配置 table，由 C++ 端读取
demo_config = {
    app_name = "lua-cpp-demo",
    max_connections = 100,
    timeout = 30.5,
    enabled = true,
    tags = { "lua", "cpp", "embed" },
}

print("test1.lua loaded successfully")
