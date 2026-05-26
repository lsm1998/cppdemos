import user;
import user_reflect;

import <any>;
import <iostream>;
import <string>;

int main()
{
    User user;

    auto reflect = make_user_reflect();

    reflect.set(user, "id", 1001);
    reflect.set(user, "name", std::string("Tom"));
    reflect.set(user, "age", 20);

    std::cout << std::any_cast<int>(reflect.get(user, "id")) << std::endl;
    std::cout << std::any_cast<std::string>(reflect.get(user, "name")) << std::endl;

    reflect.call(user, "hello");

    auto age = reflect.call(user, "add_age", {5});
    std::cout << std::any_cast<int>(age) << std::endl;

    auto info = reflect.call(user, "info");
    std::cout << std::any_cast<std::string>(info) << std::endl;

    return 0;
}
