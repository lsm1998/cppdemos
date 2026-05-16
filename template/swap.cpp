#include <iostream>
#include <type_traits>

template <typename T>
void swap(T& a, T& b)
{
    if constexpr (std::is_same_v<T, int>)
    {
        if (&a == &b)
            return;
        a ^= b;
        b ^= a;
        a ^= b;
    }
    else
    {
        T temp = a;
        a = b;
        b = temp;
    }
}

int main()
{
    int a = 1, b = 100;
    std::cout << "a=" << a << ",b=" << b << std::endl;
    swap(a, b);
    std::cout << "a=" << a << ",b=" << b << std::endl;

    float c = 1.15, d = 3.14;
    std::cout << "c=" << c << ",d=" << d << std::endl;
    swap(c, d);
    std::cout << "c=" << c << ",d=" << d << std::endl;
    return 0;
}
