#include <meta>
#include <print>
#include <array>
#include <string>

struct Person
{
    int age;
    double height;
};

struct FieldInfo
{
    std::string type;
    std::string_view name;
};

constexpr auto fields = [] {

    auto ms =
        std::meta::nonstatic_data_members_of(
            ^^Person,
            std::meta::access_context::current()
        );

    std::array<FieldInfo, 2> result{};

    for (std::size_t i = 0; i < ms.size(); ++i)
    {
        auto member = ms[i];

        result[i] = FieldInfo{

            std::string(
                std::meta::display_string_of(
                    std::meta::type_of(member)
                )
            ),

            std::meta::identifier_of(member)
        };
    }

    return result;

}();

int main()
{
    for (auto const& f : fields)
    {
        std::println("{} {}", f.type, f.name);
    }
}
