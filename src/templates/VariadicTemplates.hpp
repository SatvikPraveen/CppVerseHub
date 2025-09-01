// File: src/templates/VariadicTemplates.hpp
// Variadic template demonstrations and parameter pack techniques

#ifndef VARIADIC_TEMPLATES_HPP
#define VARIADIC_TEMPLATES_HPP

#include <tuple>
#include <utility>
#include <type_traits>
#include <functional>
#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <sstream>
#include <memory>
#include <optional>
#include <variant>

namespace CppVerseHub {
namespace Templates {
namespace Variadic {

// ===== Basic Variadic Templates =====

/**
 * @brief Simple variadic function to print multiple arguments
 */
template<typename... Args>
void print(Args&&... args) {
    ((std::cout << args << " "), ...);
    std::cout << std::endl;
}

/**
 * @brief Recursive variadic template (pre-C++17 style)
 */
template<typename T>
void print_recursive(T&& t) {
    std::cout << t << std::endl;
}

template<typename T, typename... Args>
void print_recursive(T&& t, Args&&... args) {
    std::cout << t << " ";
    print_recursive(std::forward<Args>(args)...);
}

/**
 * @brief Count number of arguments
 */
template<typename... Args>
constexpr std::size_t count_args(Args&&...) {
    return sizeof...(Args);
}

/**
 * @brief Sum all numeric arguments
 */
template<typename... Args>
constexpr auto sum(Args&&... args) {
    return (args + ...);
}

/**
 * @brief Product of all numeric arguments
 */
template<typename... Args>
constexpr auto product(Args&&... args) {
    return (args * ...);
}

/**
 * @brief Logical AND of all boolean arguments
 */
template<typename... Args>
constexpr bool all_true(Args&&... args) {
    return (args && ...);
}

/**
 * @brief Logical OR of all boolean arguments
 */
template<typename... Args>
constexpr bool any_true(Args&&... args) {
    return (args || ...);
}

// ===== Advanced Variadic Techniques =====

/**
 * @brief Variadic minimum function
 */
template<typename T>
constexpr T min_variadic(T&& t) {
    return std::forward<T>(t);
}

template<typename T, typename... Args>
constexpr auto min_variadic(T&& t, Args&&... args) {
    auto rest_min = min_variadic(std::forward<Args>(args)...);
    return t < rest_min ? std::forward<T>(t) : rest_min;
}

/**
 * @brief C++17 fold expression version
 */
template<typename... Args>
constexpr auto min_fold(Args&&... args) {
    return (args < ...);
}

/**
 * @brief Variadic maximum function
 */
template<typename... Args>
constexpr auto max_variadic(Args&&... args) {
    return (args > ...);
}

/**
 * @brief Check if all arguments are the same type
 */
template<typename T, typename... Args>
constexpr bool all_same_type_v = (std::is_same_v<T, Args> && ...);

/**
 * @brief Check if all arguments are convertible to a type
 */
template<typename Target, typename... Args>
constexpr bool all_convertible_v = (std::is_convertible_v<Args, Target> && ...);

/**
 * @brief Variadic type list utilities
 */
template<typename... Types>
struct type_list {
    static constexpr std::size_t size = sizeof...(Types);
};

/**
 * @brief Get the first type from a pack
 */
template<typename... Types>
struct first_type;

template<typename First, typename... Rest>
struct first_type<First, Rest...> {
    using type = First;
};

template<typename... Types>
using first_type_t = typename first_type<Types...>::type;

/**
 * @brief Get the last type from a pack
 */
template<typename... Types>
struct last_type;

template<typename T>
struct last_type<T> {
    using type = T;
};

template<typename First, typename... Rest>
struct last_type<First, Rest...> {
    using type = typename last_type<Rest...>::type;
};

template<typename... Types>
using last_type_t = typename last_type<Types...>::type;

/**
 * @brief Check if a type exists in a parameter pack
 */
template<typename T, typename... Types>
constexpr bool contains_type_v = (std::is_same_v<T, Types> || ...);

/**
 * @brief Get index of type in parameter pack
 */
template<typename T, typename... Types>
constexpr std::size_t type_index_v = []() -> std::size_t {
    std::size_t index = 0;
    bool found = false;
    ((std::is_same_v<T, Types> ? (found = true) : (found ? false : ++index)), ...);
    return found ? index : sizeof...(Types);
}();

// ===== Variadic Class Templates =====

/**
 * @brief Variadic tuple-like class
 */
template<typename... Types>
class Tuple;

template<>
class Tuple<> {
public:
    static constexpr std::size_t size() { return 0; }
};

template<typename Head, typename... Tail>
class Tuple<Head, Tail...> : private Tuple<Tail...> {
private:
    Head head_;
    using Base = Tuple<Tail...>;
    
public:
    static constexpr std::size_t size() { return 1 + Base::size(); }
    
    Tuple() = default;
    
    template<typename H, typename... T>
    explicit Tuple(H&& h, T&&... t) 
        : Base(std::forward<T>(t)...), head_(std::forward<H>(h)) {}
    
    Head& head() & { return head_; }
    const Head& head() const& { return head_; }
    Head&& head() && { return std::move(head_); }
    const Head&& head() const&& { return std::move(head_); }
    
    Base& tail() { return static_cast<Base&>(*this); }
    const Base& tail() const { return static_cast<const Base&>(*this); }
};

/**
 * @brief Get element from tuple by index
 */
template<std::size_t Index, typename... Types>
constexpr auto& get(Tuple<Types...>& t) {
    if constexpr (Index == 0) {
        return t.head();
    } else {
        return get<Index - 1>(t.tail());
    }
}

template<std::size_t Index, typename... Types>
constexpr const auto& get(const Tuple<Types...>& t) {
    if constexpr (Index == 0) {
        return t.head();
    } else {
        return get<Index - 1>(t.tail());
    }
}

/**
 * @brief Make tuple helper
 */
template<typename... Types>
constexpr auto make_tuple(Types&&... args) {
    return Tuple<std::decay_t<Types>...>(std::forward<Types>(args)...);
}

/**
 * @brief Variadic visitor pattern
 */
template<typename... Visitors>
struct overload : Visitors... {
    using Visitors::operator()...;
};

template<typename... Visitors>
overload(Visitors...) -> overload<Visitors...>;

/**
 * @brief Variadic function object
 */
template<typename... Funcs>
class multifunction;

template<typename Func>
class multifunction<Func> {
private:
    Func func_;
    
public:
    explicit multifunction(Func f) : func_(std::move(f)) {}
    
    template<typename... Args>
    auto operator()(Args&&... args) const {
        return func_(std::forward<Args>(args)...);
    }
};

template<typename Func, typename... Funcs>
class multifunction<Func, Funcs...> : private multifunction<Funcs...> {
private:
    Func func_;
    using Base = multifunction<Funcs...>;
    
public:
    explicit multifunction(Func f, Funcs... fs) 
        : Base(std::move(fs)...), func_(std::move(f)) {}
    
    template<typename... Args>
    auto operator()(Args&&... args) const {
        if constexpr (std::is_invocable_v<Func, Args...>) {
            return func_(std::forward<Args>(args)...);
        } else {
            return Base::operator()(std::forward<Args>(args)...);
        }
    }
};

/**
 * @brief Variadic hash combiner
 */
template<typename... Types>
struct hash_combine {
    std::size_t operator()(const Types&... values) const {
        std::size_t seed = 0;
        ((seed ^= std::hash<Types>{}(values) + 0x9e3779b9 + (seed << 6) + (seed >> 2)), ...);
        return seed;
    }
};

/**
 * @brief Variadic factory class
 */
template<typename Product, typename... Args>
class Factory {
private:
    std::tuple<Args...> args_;
    
public:
    explicit Factory(Args... args) : args_(std::move(args)...) {}
    
    template<typename... ExtraArgs>
    Product create(ExtraArgs&&... extra_args) const {
        return std::apply([&](const auto&... stored_args) {
            return Product(stored_args..., std::forward<ExtraArgs>(extra_args)...);
        }, args_);
    }
};

/**
 * @brief Variadic builder pattern
 */
template<typename Product>
class VariadicBuilder {
private:
    std::tuple<> data_;
    
public:
    VariadicBuilder() = default;
    
    template<typename T>
    auto with(T&& value) && {
        auto new_data = std::tuple_cat(std::move(data_), std::make_tuple(std::forward<T>(value)));
        
        class NewBuilder {
        private:
            decltype(new_data) data_;
            
        public:
            explicit NewBuilder(decltype(new_data) d) : data_(std::move(d)) {}
            
            template<typename U>
            auto with(U&& val) && {
                return VariadicBuilder<Product>{}.init_with_tuple(
                    std::tuple_cat(std::move(data_), std::make_tuple(std::forward<U>(val)))
                );
            }
            
            Product build() && {
                return std::apply([](auto&&... args) {
                    return Product(std::forward<decltype(args)>(args)...);
                }, std::move(data_));
            }
            
            template<typename Tuple>
            auto init_with_tuple(Tuple&& t) {
                class TupleBuilder {
                private:
                    Tuple data_;
                    
                public:
                    explicit TupleBuilder(Tuple d) : data_(std::move(d)) {}
                    
                    template<typename U>
                    auto with(U&& val) && {
                        return TupleBuilder{std::tuple_cat(std::move(data_), std::make_tuple(std::forward<U>(val)))};
                    }
                    
                    Product build() && {
                        return std::apply([](auto&&... args) {
                            return Product(std::forward<decltype(args)>(args)...);
                        }, std::move(data_));
                    }
                };
                return TupleBuilder{std::forward<Tuple>(t)};
            }
        };
        
        return NewBuilder{std::move(new_data)};
    }
};

// ===== Variadic Algorithms =====

/**
 * @brief Apply function to each argument
 */
template<typename Func, typename... Args>
void for_each_arg(Func&& func, Args&&... args) {
    (func(std::forward<Args>(args)), ...);
}

/**
 * @brief Transform each argument and collect results
 */
template<typename Func, typename... Args>
constexpr auto transform_args(Func&& func, Args&&... args) {
    return std::make_tuple(func(std::forward<Args>(args))...);
}

/**
 * @brief Filter arguments based on predicate
 */
template<typename Pred, typename... Args>
constexpr auto filter_args(Pred&& pred, Args&&... args) {
    return std::tuple_cat(
        std::conditional_t<
            std::invoke_result_v<Pred, Args>,
            std::tuple<std::decay_t<Args>>,
            std::tuple<>
        >(args)...
    );
}

/**
 * @brief Zip multiple parameter packs
 */
template<std::size_t... Indices, typename... Tuples>
constexpr auto zip_impl(std::index_sequence<Indices...>, Tuples&&... tuples) {
    return std::make_tuple(std::make_tuple(std::get<Indices>(tuples)...)...);
}

template<typename... Tuples>
constexpr auto zip(Tuples&&... tuples) {
    constexpr auto min_size = std::min({std::tuple_size_v<std::decay_t<Tuples>>...});
    return zip_impl(std::make_index_sequence<min_size>{}, std::forward<Tuples>(tuples)...);
}

/**
 * @brief Variadic compose function
 */
template<typename F>
constexpr auto compose(F&& f) {
    return std::forward<F>(f);
}

template<typename F, typename... Funcs>
constexpr auto compose(F&& f, Funcs&&... funcs) {
    return [f = std::forward<F>(f), composed = compose(std::forward<Funcs>(funcs)...)](auto&&... args) {
        return f(composed(std::forward<decltype(args)>(args)...));
    };
}

/**
 * @brief Variadic pipeline
 */
template<typename T>
class Pipeline {
private:
    T value_;
    
public:
    explicit Pipeline(T value) : value_(std::move(value)) {}
    
    template<typename Func>
    auto then(Func&& func) && {
        auto result = func(std::move(value_));
        return Pipeline<decltype(result)>{std::move(result)};
    }
    
    T get() && { return std::move(value_); }
    const T& get() const& { return value_; }
};

template<typename T>
Pipeline<std::decay_t<T>> make_pipeline(T&& value) {
    return Pipeline<std::decay_t<T>>{std::forward<T>(value)};
}

// ===== Variadic Utilities =====

/**
 * @brief Variadic string formatter
 */
template<typename... Args>
std::string format_string(const std::string& format, Args&&... args) {
    std::ostringstream oss;
    std::size_t pos = 0;
    std::size_t arg_index = 0;
    
    auto format_arg = [&](auto&& arg) {
        std::size_t placeholder = format.find("{}", pos);
        if (placeholder != std::string::npos && arg_index < sizeof...(Args)) {
            oss << format.substr(pos, placeholder - pos) << arg;
            pos = placeholder + 2;
        }
        ++arg_index;
    };
    
    (format_arg(args), ...);
    oss << format.substr(pos);
    
    return oss.str();
}

/**
 * @brief Variadic array maker
 */
template<typename T, typename... Args>
    requires (std::is_convertible_v<Args, T> && ...)
constexpr std::array<T, sizeof...(Args)> make_array(Args&&... args) {
    return {{static_cast<T>(std::forward<Args>(args))...}};
}

/**
 * @brief Variadic vector maker
 */
template<typename... Args>
auto make_vector(Args&&... args) {
    using T = std::common_type_t<std::decay_t<Args>...>;
    return std::vector<T>{static_cast<T>(std::forward<Args>(args))...};
}

/**
 * @brief Variadic optional chain
 */
template<typename T>
constexpr std::optional<T> optional_chain(std::optional<T> opt) {
    return opt;
}

template<typename T, typename Func, typename... Funcs>
constexpr auto optional_chain(std::optional<T> opt, Func&& func, Funcs&&... funcs) {
    if (!opt) return decltype(optional_chain(func(*opt), funcs...)){};
    return optional_chain(func(*opt), std::forward<Funcs>(funcs)...);
}

/**
 * @brief Variadic variant visitor
 */
template<typename Variant, typename... Visitors>
auto visit_variant(Variant&& var, Visitors&&... visitors) {
    return std::visit(overload{std::forward<Visitors>(visitors)...}, std::forward<Variant>(var));
}

/**
 * @brief Thread-safe variadic function call
 */
template<typename Func, typename... Args>
auto safe_call(Func&& func, Args&&... args) -> std::optional<std::invoke_result_t<Func, Args...>> {
    try {
        if constexpr (std::is_void_v<std::invoke_result_t<Func, Args...>>) {
            std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
            return std::nullopt; // Placeholder for void return
        } else {
            return std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
        }
    } catch (...) {
        return std::nullopt;
    }
}

/**
 * @brief Variadic perfect forwarding wrapper
 */
template<typename Func>
class perfect_forwarder {
private:
    Func func_;
    
public:
    explicit perfect_forwarder(Func func) : func_(std::move(func)) {}
    
    template<typename... Args>
    decltype(auto) operator()(Args&&... args) const {
        return func_(std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    decltype(auto) operator()(Args&&... args) {
        return func_(std::forward<Args>(args)...);
    }
};

template<typename Func>
perfect_forwarder<std::decay_t<Func>> make_perfect_forwarder(Func&& func) {
    return perfect_forwarder<std::decay_t<Func>>{std::forward<Func>(func)};
}

/**
 * @brief Variadic memoization
 */
template<typename Func>
class memoized;

template<typename R, typename... Args>
class memoized<R(Args...)> {
private:
    mutable std::map<std::tuple<Args...>, R> cache_;
    std::function<R(Args...)> func_;
    
public:
    explicit memoized(std::function<R(Args...)> f) : func_(std::move(f)) {}
    
    R operator()(Args... args) const {
        auto key = std::make_tuple(args...);
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return it->second;
        }
        
        auto result = func_(args...);
        cache_[key] = result;
        return result;
    }
};

template<typename Func>
auto memoize(Func&& func) {
    return memoized<std::decay_t<Func>>{std::forward<Func>(func)};
}

} // namespace Variadic
} // namespace Templates
} // namespace CppVerseHub

#endif // VARIADIC_TEMPLATES_HPP