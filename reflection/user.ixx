export module user;

import <iostream>;
import <string>;

export struct User
{
    int id = 0;
    std::string name;
    int age = 0;

    void hello() { std::cout << "hello, " << name << std::endl; }

    int add_age(int value)
    {
        age += value;
        return age;
    }

    std::string info() const
    {
        return "User{id=" + std::to_string(id) + ", name=" + name + ", age=" + std::to_string(age) + "}";
    }
};
