export module reflect;

import <any>;
import <functional>;
import <stdexcept>;
import <string>;
import <unordered_map>;
import <vector>;

export template <typename T>
class Reflect
{
public:
    using Getter = std::function<std::any(T&)>;
    using Setter = std::function<void(T&, std::any)>;
    using Method = std::function<std::any(T&, const std::vector<std::any>&)>;

    void field(const std::string& name, Getter getter, Setter setter)
    {
        getters_[name] = std::move(getter);
        setters_[name] = std::move(setter);
    }

    void method(const std::string& name, Method method) { methods_[name] = std::move(method); }

    std::any get(T& obj, const std::string& name) const
    {
        auto it = getters_.find(name);
        if (it == getters_.end())
        {
            throw std::runtime_error("field not found: " + name);
        }
        return it->second(obj);
    }

    void set(T& obj, const std::string& name, std::any value) const
    {
        auto it = setters_.find(name);
        if (it == setters_.end())
        {
            throw std::runtime_error("field not found: " + name);
        }
        it->second(obj, std::move(value));
    }

    std::any call(T& obj, const std::string& name, const std::vector<std::any>& args = {}) const
    {
        auto it = methods_.find(name);
        if (it == methods_.end())
        {
            throw std::runtime_error("method not found: " + name);
        }
        return it->second(obj, args);
    }

private:
    std::unordered_map<std::string, Getter> getters_;
    std::unordered_map<std::string, Setter> setters_;
    std::unordered_map<std::string, Method> methods_;
};
