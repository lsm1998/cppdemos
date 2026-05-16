#include <cstdio>
#include <stdio.h>
#include <vector>

int main()
{
    std::vector<int> list = {100, 10};
    int* p = &list[0];
    printf("p 指向的地址: %p\n", p);
    for (int i = 0; i < 100; i++)
    {
        list.push_back(i);
    }
    p = &list[0];
    printf("p 指向的地址: %p\n", p);
    return 0;
}
