export module user_reflect;

import reflect;
import user;

import <any>;
import <string>;
import <vector>;

export Reflect<User> make_user_reflect()
{
    Reflect<User> r;

    r.field("id", [](User& u) -> std::any { return u.id; }, [](User& u, std::any v) { u.id = std::any_cast<int>(v); });

    r.field(
        "name", [](User& u) -> std::any { return u.name; },
        [](User& u, std::any v) { u.name = std::any_cast<std::string>(v); });

    r.field(
        "age", [](User& u) -> std::any { return u.age; }, [](User& u, std::any v) { u.age = std::any_cast<int>(v); });

    r.method("hello",
             [](User& u, const std::vector<std::any>&) -> std::any
             {
                 u.hello();
                 return {};
             });

    r.method("add_age",
             [](User& u, const std::vector<std::any>& args) -> std::any
             {
                 int value = std::any_cast<int>(args.at(0));
                 return u.add_age(value);
             });

    r.method("info", [](User& u, const std::vector<std::any>&) -> std::any { return u.info(); });

    return r;
}
