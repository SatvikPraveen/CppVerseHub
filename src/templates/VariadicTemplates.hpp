/**
 * @file VariadicTemplates.hpp
 * @brief Variadic templates: parameter packs, pack expansion and fold expressions.
 *
 * Demonstrates
 *  - recursive pack processing (pre-C++17) vs. fold expressions (C++17);
 *  - pack introspection (`sizeof...`, first/last/index-of type);
 *  - a recursive-inheritance tuple (`RecursiveTuple`) and index-based access;
 *  - the `overload` idiom (`using Ts::operator()...`) vs. a first-match `multifunction`;
 *  - perfect forwarding of packs, storing packs in tuples and replaying them with std::apply
 *    (`Factory`, `Builder`, `make_pipeline`, `compose`);
 *  - nested pack expansion (`zip`), monadic chaining of std::optional, memoisation keyed by a pack.
 */

#ifndef CPPVERSEHUB_TEMPLATES_VARIADIC_TEMPLATES_HPP
#define CPPVERSEHUB_TEMPLATES_VARIADIC_TEMPLATES_HPP

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace CppVerseHub::Templates::Variadic {

// ===== Printing: fold vs. recursion =====

/**
 * @brief Print arguments separated by spaces using a comma fold.
 * @param out destination stream
 * @param args values to print
 */
template <typename... Args>
void print(std::ostream& out, const Args&... args) {
    [[maybe_unused]] std::size_t index = 0;
    ((out << (index++ == 0 ? "" : " ") << args), ...);
    out << '\n';
}

/** @brief Recursion base case. @param out stream @param last final value */
template <typename T>
void print_recursive(std::ostream& out, const T& last) {
    out << last << '\n';
}

/** @brief Peel off the head and recurse (pre-C++17 style). @param out stream @param head first @param tail rest */
template <typename T, typename... Rest>
void print_recursive(std::ostream& out, const T& head, const Rest&... tail) {
    out << head << ' ';
    print_recursive(out, tail...);
}

/**
 * @brief Join arguments into a string with a separator.
 * @param separator text between items
 * @param args values
 * @return joined text
 */
template <typename... Args>
[[nodiscard]] std::string join([[maybe_unused]] std::string_view separator, const Args&... args) {
    std::ostringstream oss;
    [[maybe_unused]] std::size_t index = 0;
    ((oss << (index++ == 0 ? std::string_view{} : separator) << args), ...);
    return oss.str();
}

// ===== Fold-expression arithmetic =====

/** @return number of arguments */
template <typename... Args>
[[nodiscard]] constexpr std::size_t count_args(const Args&...) noexcept {
    return sizeof...(Args);
}

/** @param args at least one addend @return right fold of + */
template <typename... Args>
    requires(sizeof...(Args) > 0)
[[nodiscard]] constexpr auto sum(const Args&... args) {
    return (args + ...);
}

/** @param args factors (empty product is 1) @return left fold of * with identity */
template <typename... Args>
[[nodiscard]] constexpr auto product(const Args&... args) {
    return (1 * ... * args);
}

/** @param args values @return true if all are truthy (true for an empty pack) */
template <typename... Args>
[[nodiscard]] constexpr bool all_true(const Args&... args) {
    return (static_cast<bool>(args) && ...);
}

/** @param args values @return true if any is truthy (false for an empty pack) */
template <typename... Args>
[[nodiscard]] constexpr bool any_true(const Args&... args) {
    return (static_cast<bool>(args) || ...);
}

/** @brief Recursive minimum: base case. @param value only value @return value */
template <typename T>
[[nodiscard]] constexpr T min_recursive(const T& value) {
    return value;
}

/** @brief Recursive minimum. @param first head @param rest tail @return smallest value (common type) */
template <typename T, typename... Rest>
[[nodiscard]] constexpr std::common_type_t<T, Rest...> min_recursive(const T& first, const Rest&... rest) {
    using R = std::common_type_t<T, Rest...>;
    const R tail_min = min_recursive(static_cast<R>(rest)...);
    return static_cast<R>(first) < tail_min ? static_cast<R>(first) : tail_min;
}

/** @brief Minimum via a comma fold over assignments. @param first head @param rest tail @return smallest */
template <typename T, typename... Rest>
[[nodiscard]] constexpr std::common_type_t<T, Rest...> min_fold(const T& first, const Rest&... rest) {
    std::common_type_t<T, Rest...> result = first;
    ((result = rest < result ? rest : result), ...);
    return result;
}

/** @brief Maximum via a comma fold. @param first head @param rest tail @return largest */
template <typename T, typename... Rest>
[[nodiscard]] constexpr std::common_type_t<T, Rest...> max_fold(const T& first, const Rest&... rest) {
    std::common_type_t<T, Rest...> result = first;
    ((result = result < rest ? rest : result), ...);
    return result;
}

// ===== Pack introspection =====

/** @brief True if every type in Args is T. */
template <typename T, typename... Args>
inline constexpr bool all_same_type_v = (std::is_same_v<T, Args> && ...);

/** @brief True if every type in Args converts to Target. */
template <typename Target, typename... Args>
inline constexpr bool all_convertible_v = (std::is_convertible_v<Args, Target> && ...);

/** @brief First type of a non-empty pack. */
template <typename... Types>
struct first_type;
/** @brief Implementation. */
template <typename First, typename... Rest>
struct first_type<First, Rest...> {
    using type = First;
};
/** @brief Alias. */
template <typename... Types>
using first_type_t = typename first_type<Types...>::type;

/** @brief Last type of a non-empty pack (via a fold over the comma-separated type identities). */
template <typename... Types>
struct last_type {
    using type = typename decltype((std::type_identity<Types>{}, ...))::type;
};
/** @brief Alias. */
template <typename... Types>
using last_type_t = typename last_type<Types...>::type;

/** @brief True if T appears in Types. */
template <typename T, typename... Types>
inline constexpr bool contains_type_v = (std::is_same_v<T, Types> || ...);

/** @brief Index of the first T in Types, or sizeof...(Types) if absent. */
template <typename T, typename... Types>
inline constexpr std::size_t type_index_v = [] {
    constexpr std::array<bool, sizeof...(Types) + 1> matches{std::is_same_v<T, Types>..., true};
    std::size_t index = 0;
    while (!matches[index]) {
        ++index;
    }
    return index;
}();

/** @brief Number of occurrences of T in Types. */
template <typename T, typename... Types>
inline constexpr std::size_t count_type_v = (std::size_t{0} + ... + (std::is_same_v<T, Types> ? 1U : 0U));

// ===== Recursive-inheritance tuple =====

/** @brief A tuple built by inheriting from the tuple of the tail types. */
template <typename... Types>
class RecursiveTuple;

/** @brief Empty tuple terminates the recursion. */
template <>
class RecursiveTuple<> {
public:
    /** @return 0 */
    [[nodiscard]] static constexpr std::size_t size() noexcept { return 0; }
};

/** @brief Head element + base holding the tail. */
template <typename Head, typename... Tail>
class RecursiveTuple<Head, Tail...> : private RecursiveTuple<Tail...> {
    using Base = RecursiveTuple<Tail...>;

public:
    /** @return number of elements */
    [[nodiscard]] static constexpr std::size_t size() noexcept { return 1 + sizeof...(Tail); }

    /** @brief Value-initialise every element. */
    constexpr RecursiveTuple() = default;

    /** @brief Element-wise construction (exactly one argument per element). @param h head @param t tail */
    template <typename H, typename... T>
        requires(sizeof...(T) == sizeof...(Tail) && !std::is_same_v<std::remove_cvref_t<H>, RecursiveTuple>)
    constexpr explicit RecursiveTuple(H&& h, T&&... t) : Base(std::forward<T>(t)...), head_(std::forward<H>(h)) {}

    /** @return head element */
    [[nodiscard]] constexpr Head& head() & noexcept { return head_; }
    /** @return head element */
    [[nodiscard]] constexpr const Head& head() const& noexcept { return head_; }
    /** @return tail tuple */
    [[nodiscard]] constexpr Base& tail() noexcept { return *this; }
    /** @return tail tuple */
    [[nodiscard]] constexpr const Base& tail() const noexcept { return *this; }

private:
    Head head_{};
};

/** @brief Index-based access (recursion on the index). @param t tuple @return element Index */
template <std::size_t Index, typename... Types>
    requires(Index < sizeof...(Types))
[[nodiscard]] constexpr auto& get(RecursiveTuple<Types...>& t) noexcept {
    if constexpr (Index == 0) {
        return t.head();
    } else {
        return get<Index - 1>(t.tail());
    }
}

/** @brief Const index-based access. @param t tuple @return element Index */
template <std::size_t Index, typename... Types>
    requires(Index < sizeof...(Types))
[[nodiscard]] constexpr const auto& get(const RecursiveTuple<Types...>& t) noexcept {
    if constexpr (Index == 0) {
        return t.head();
    } else {
        return get<Index - 1>(t.tail());
    }
}

/** @param args elements @return RecursiveTuple of decayed argument types */
template <typename... Types>
[[nodiscard]] constexpr auto make_recursive_tuple(Types&&... args) {
    return RecursiveTuple<std::decay_t<Types>...>(std::forward<Types>(args)...);
}

// ===== Overload sets from lambdas =====

/** @brief Inherit call operators from every lambda; overload resolution picks the best match. */
template <typename... Visitors>
struct overload : Visitors... {
    using Visitors::operator()...;
};

/** @brief Deduction guide (unnecessary since C++20 aggregate CTAD, kept for clarity/portability). */
template <typename... Visitors>
overload(Visitors...) -> overload<Visitors...>;

/** @brief First-match dispatcher: tries callables in order (contrast with overload's best-match). */
template <typename... Funcs>
class multifunction;

/** @brief Last callable. */
template <typename Func>
class multifunction<Func> {
public:
    /** @param f callable */
    explicit multifunction(Func f) : func_(std::move(f)) {}
    /** @param args arguments @return f(args...) */
    template <typename... Args>
        requires std::invocable<const Func&, Args...>
    decltype(auto) operator()(Args&&... args) const {
        return std::invoke(func_, std::forward<Args>(args)...);
    }

private:
    Func func_;
};

/** @brief Try Func, else delegate to the rest. */
template <typename Func, typename... Funcs>
class multifunction<Func, Funcs...> : private multifunction<Funcs...> {
    using Base = multifunction<Funcs...>;

public:
    /** @param f first callable @param fs remaining callables */
    explicit multifunction(Func f, Funcs... fs) : Base(std::move(fs)...), func_(std::move(f)) {}
    /** @param args arguments @return result of the first callable invocable with args */
    template <typename... Args>
    decltype(auto) operator()(Args&&... args) const {
        if constexpr (std::is_invocable_v<const Func&, Args...>) {
            return std::invoke(func_, std::forward<Args>(args)...);
        } else {
            return Base::operator()(std::forward<Args>(args)...);
        }
    }

private:
    Func func_;
};

/** @brief CTAD guide. */
template <typename... Funcs>
multifunction(Funcs...) -> multifunction<Funcs...>;

/** @brief Boost-style hash_combine over a pack of hashable values. */
template <typename... Types>
struct hash_combine {
    /** @param values values to hash @return combined hash */
    [[nodiscard]] std::size_t operator()(const Types&... values) const {
        std::size_t seed = 0;
        ((seed ^= std::hash<Types>{}(values) + 0x9e3779b9U + (seed << 6U) + (seed >> 2U)), ...);
        return seed;
    }
};

// ===== Storing and replaying packs =====

/**
 * @brief Stores constructor arguments and creates Products on demand, optionally appending more.
 * @tparam Product type to create
 * @tparam Args stored argument types
 */
template <typename Product, typename... Args>
class Factory {
public:
    /** @param args arguments stored by value */
    explicit Factory(Args... args) : args_(std::move(args)...) {}

    /** @param extra_args appended arguments @return Product(stored..., extra...) */
    template <typename... ExtraArgs>
    [[nodiscard]] Product create(ExtraArgs&&... extra_args) const {
        return std::apply(
            [&](const auto&... stored) { return Product(stored..., std::forward<ExtraArgs>(extra_args)...); }, args_);
    }

private:
    std::tuple<Args...> args_;
};

/**
 * @brief Type-accumulating builder: every with() returns a new Builder type whose pack grows by one.
 * @tparam Product type to construct
 * @tparam Fields accumulated argument types
 */
template <typename Product, typename... Fields>
class Builder {
public:
    /** @brief Empty builder. */
    Builder() = default;
    /** @param fields accumulated arguments */
    explicit Builder(std::tuple<Fields...> fields) : fields_(std::move(fields)) {}

    /** @param value next constructor argument @return builder with one more field */
    template <typename T>
    [[nodiscard]] auto with(T&& value) && {
        return Builder<Product, Fields..., std::decay_t<T>>(
            std::tuple_cat(std::move(fields_), std::make_tuple(std::forward<T>(value))));
    }

    /** @return Product(fields...) */
    [[nodiscard]] Product build() && {
        return std::make_from_tuple<Product>(std::move(fields_));
    }

    /** @return number of accumulated fields */
    [[nodiscard]] static constexpr std::size_t field_count() noexcept { return sizeof...(Fields); }

private:
    std::tuple<Fields...> fields_;
};

/** @return empty Builder for Product */
template <typename Product>
[[nodiscard]] Builder<Product> make_builder() {
    return Builder<Product>{};
}

// ===== Pack algorithms =====

/** @brief Call func on each argument, in order. @param func callable @param args arguments */
template <typename Func, typename... Args>
constexpr void for_each_arg(Func&& func, Args&&... args) {
    (std::invoke(func, std::forward<Args>(args)), ...);
}

/** @param func callable @param args arguments @return tuple of func(arg) for each arg */
template <typename Func, typename... Args>
[[nodiscard]] constexpr auto transform_args(Func&& func, Args&&... args) {
    return std::make_tuple(std::invoke(func, std::forward<Args>(args))...);
}

namespace detail {
template <template <typename> class Pred, typename Arg>
constexpr auto keep_if(Arg&& arg) {
    using T = std::decay_t<Arg>;
    if constexpr (Pred<T>::value) {
        return std::tuple<T>(std::forward<Arg>(arg));
    } else {
        return std::tuple<>{};
    }
}
} // namespace detail

/**
 * @brief Keep the arguments whose decayed type satisfies the trait Pred (e.g. std::is_integral).
 * @param args arguments
 * @return tuple of kept (decayed) arguments
 */
template <template <typename> class Pred, typename... Args>
[[nodiscard]] constexpr auto filter_args(Args&&... args) {
    return std::tuple_cat(detail::keep_if<Pred>(std::forward<Args>(args))...);
}

namespace detail {
template <std::size_t I, typename... Tuples>
constexpr auto zip_at(const Tuples&... tuples) {
    return std::make_tuple(std::get<I>(tuples)...);
}
} // namespace detail

/**
 * @brief Zip tuples element-wise up to the shortest length. Uses nested expansion: the inner
 *        `tuples...` expands per index, the outer `I...` builds the result.
 * @param tuples tuple-like objects
 * @return tuple of tuples
 */
template <typename... Tuples>
    requires(sizeof...(Tuples) > 0)
[[nodiscard]] constexpr auto zip(const Tuples&... tuples) {
    constexpr std::size_t length = std::min({std::tuple_size_v<Tuples>...});
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
        return std::make_tuple(detail::zip_at<I>(tuples...)...);
    }(std::make_index_sequence<length>{});
}

/** @brief compose(f) == f. @param f callable @return f */
template <typename F>
[[nodiscard]] constexpr auto compose(F&& f) {
    return std::forward<F>(f);
}

/** @brief compose(f, g, h)(x) == f(g(h(x))). @param f outermost @param funcs inner callables @return composite */
template <typename F, typename... Funcs>
[[nodiscard]] constexpr auto compose(F&& f, Funcs&&... funcs) {
    return [outer = std::forward<F>(f), inner = compose(std::forward<Funcs>(funcs)...)](auto&&... args) {
        return outer(inner(std::forward<decltype(args)>(args)...));
    };
}

/**
 * @brief Value-carrying pipeline; each then() may change the value type.
 * @tparam T current value type
 */
template <typename T>
class Pipeline {
public:
    /** @param value initial value */
    explicit Pipeline(T value) : value_(std::move(value)) {}

    /** @param func transformation @return pipeline over func(value) */
    template <typename Func>
    [[nodiscard]] auto then(Func&& func) && {
        using R = std::decay_t<std::invoke_result_t<Func, T&&>>;
        return Pipeline<R>(std::invoke(std::forward<Func>(func), std::move(value_)));
    }

    /** @return final value (moved out) */
    [[nodiscard]] T get() && { return std::move(value_); }
    /** @return final value */
    [[nodiscard]] const T& get() const& { return value_; }

private:
    T value_;
};

/** @param value initial value @return pipeline */
template <typename T>
[[nodiscard]] Pipeline<std::decay_t<T>> make_pipeline(T&& value) {
    return Pipeline<std::decay_t<T>>(std::forward<T>(value));
}

// ===== Utilities =====

/**
 * @brief Replace successive "{}" placeholders with the arguments (extra args are ignored,
 *        missing args leave the placeholder in place).
 * @param format pattern
 * @param args substitutions
 * @return formatted text
 */
template <typename... Args>
[[nodiscard]] std::string format_string(std::string_view format, const Args&... args) {
    std::ostringstream oss;
    std::size_t pos = 0;
    const auto emit = [&](const auto& arg) {
        const std::size_t placeholder = format.find("{}", pos);
        if (placeholder == std::string_view::npos) {
            return;
        }
        oss << format.substr(pos, placeholder - pos) << arg;
        pos = placeholder + 2;
    };
    (emit(args), ...);
    oss << format.substr(pos);
    return oss.str();
}

/** @param args values convertible to T @return std::array<T, N> */
template <typename T, typename... Args>
    requires(std::is_convertible_v<Args, T> && ...)
[[nodiscard]] constexpr std::array<T, sizeof...(Args)> make_array(Args&&... args) {
    return {{static_cast<T>(std::forward<Args>(args))...}};
}

/** @param args at least one value @return vector of the common type */
template <typename... Args>
    requires(sizeof...(Args) > 0)
[[nodiscard]] auto make_vector(Args&&... args) {
    using T = std::common_type_t<std::decay_t<Args>...>;
    std::vector<T> result;
    result.reserve(sizeof...(Args));
    (result.push_back(static_cast<T>(std::forward<Args>(args))), ...);
    return result;
}

/** @brief Chain base case. @param opt value @return opt */
template <typename T>
[[nodiscard]] constexpr std::optional<T> optional_chain(std::optional<T> opt) {
    return opt;
}

/**
 * @brief Monadic chaining: each func maps a value to std::optional<U>; the first empty result
 *        short-circuits the rest.
 * @param opt starting value
 * @param func next step
 * @param funcs remaining steps
 * @return final optional
 */
template <typename T, typename Func, typename... Funcs>
[[nodiscard]] constexpr auto optional_chain(std::optional<T> opt, Func&& func, Funcs&&... funcs) {
    using Next = std::invoke_result_t<Func, T&>;
    using Result = decltype(optional_chain(std::declval<Next>(), std::declval<Funcs>()...));
    if (!opt) {
        return Result{};
    }
    return optional_chain(std::invoke(func, *opt), std::forward<Funcs>(funcs)...);
}

/** @param var variant @param visitors lambdas, one per alternative @return visitor result */
template <typename Variant, typename... Visitors>
decltype(auto) visit_variant(Variant&& var, Visitors&&... visitors) {
    return std::visit(overload{std::forward<Visitors>(visitors)...}, std::forward<Variant>(var));
}

/**
 * @brief Exception-safe call. For non-void results returns std::optional (empty on exception);
 *        for void results returns bool (false on exception).
 * @param func callable
 * @param args arguments
 * @return optional result or success flag
 */
template <typename Func, typename... Args>
[[nodiscard]] auto safe_call(Func&& func, Args&&... args) noexcept {
    using R = std::invoke_result_t<Func, Args...>;
    if constexpr (std::is_void_v<R>) {
        try {
            std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
            return true;
        } catch (...) {
            return false;
        }
    } else {
        using Result = std::optional<std::decay_t<R>>;
        try {
            return Result(std::invoke(std::forward<Func>(func), std::forward<Args>(args)...));
        } catch (...) {
            return Result{};
        }
    }
}

/**
 * @brief Wraps a callable and perfectly forwards every call to it.
 * @tparam Func wrapped callable
 */
template <typename Func>
class perfect_forwarder {
public:
    /** @param func callable */
    explicit perfect_forwarder(Func func) : func_(std::move(func)) {}
    /** @param args arguments @return func(args...) */
    template <typename... Args>
    decltype(auto) operator()(Args&&... args) const {
        return std::invoke(func_, std::forward<Args>(args)...);
    }
    /** @param args arguments @return func(args...) */
    template <typename... Args>
    decltype(auto) operator()(Args&&... args) {
        return std::invoke(func_, std::forward<Args>(args)...);
    }

private:
    Func func_;
};

/** @param func callable @return forwarding wrapper */
template <typename Func>
[[nodiscard]] perfect_forwarder<std::decay_t<Func>> make_perfect_forwarder(Func&& func) {
    return perfect_forwarder<std::decay_t<Func>>(std::forward<Func>(func));
}

/** @brief Memoising wrapper keyed by the decayed argument pack. Not thread-safe. */
template <typename Signature>
class memoized;

/** @brief Implementation for R(Args...). */
template <typename R, typename... Args>
class memoized<R(Args...)> {
public:
    using key_type = std::tuple<std::decay_t<Args>...>; ///< cache key

    /** @param f function to memoise */
    explicit memoized(std::function<R(Args...)> f) : func_(std::move(f)) {}

    /** @param args arguments @return cached or freshly computed result */
    R operator()(Args... args) {
        key_type key(args...);
        if (const auto it = cache_.find(key); it != cache_.end()) {
            ++hits_;
            return it->second;
        }
        R result = func_(std::forward<Args>(args)...);
        cache_.emplace(std::move(key), result);
        return result;
    }

    /** @return number of cache hits */
    [[nodiscard]] std::size_t hits() const noexcept { return hits_; }
    /** @return number of cached entries */
    [[nodiscard]] std::size_t cache_size() const noexcept { return cache_.size(); }

private:
    std::function<R(Args...)> func_;
    std::map<key_type, R> cache_;
    std::size_t hits_ = 0;
};

/**
 * @brief Memoise func with an explicit signature, e.g. `memoize<long(int)>(f)`.
 * @param func callable compatible with Signature
 * @return memoising wrapper
 */
template <typename Signature, typename Func>
[[nodiscard]] memoized<Signature> memoize(Func&& func) {
    return memoized<Signature>(std::function<Signature>(std::forward<Func>(func)));
}

// ===== Showcase =====

/**
 * @brief Exercise the variadic utilities.
 * @param out destination stream
 */
inline void demonstrate_variadic_templates(std::ostream& out = std::cout) {
    out << "--- Variadic templates ---\n";
    print(out, "print:", 1, 2.5, 'c', std::string("str"));
    out << "print_recursive:    ";
    print_recursive(out, 1, 2, 3);
    out << "sum(1,2,3,4)        = " << sum(1, 2, 3, 4) << '\n';
    out << "product(2,3,4)      = " << product(2, 3, 4) << '\n';
    out << "min_fold(5,2.5,9)   = " << min_fold(5, 2.5, 9) << '\n';
    out << "join                = " << join(", ", "a", 1, 'b') << '\n';
    out << "format_string       = " << format_string("{} + {} = {}", 2, 3, 5) << '\n';

    auto tuple = make_recursive_tuple(1, std::string("two"), 3.0);
    out << "RecursiveTuple<1>   = " << get<1>(tuple) << '\n';

    const std::variant<int, std::string> var = std::string("variant");
    out << "visit_variant       = "
        << visit_variant(
               var, [](int i) { return "int " + std::to_string(i); },
               [](const std::string& s) { return "string " + s; })
        << '\n';

    const auto inc_then_double = compose([](int x) { return x * 2; }, [](int x) { return x + 1; });
    out << "compose(x*2, x+1)(4)= " << inc_then_double(4) << '\n';

    const auto words = make_builder<std::string>().with(std::size_t{3}).with('z').build();
    out << "Builder<string>     = " << words << '\n';

    const auto result = optional_chain(
        std::optional<int>(16), [](int v) { return v > 0 ? std::optional<int>(v / 2) : std::nullopt; },
        [](int v) { return std::optional<std::string>(std::to_string(v)); });
    out << "optional_chain      = " << result.value_or("empty") << '\n';

    auto square = memoize<long(int)>([](int v) { return static_cast<long>(v) * v; });
    out << "memoized square(12) = " << square(12) << " (again: " << square(12) << ", hits " << square.hits()
        << ")\n";
}

} // namespace CppVerseHub::Templates::Variadic

#endif // CPPVERSEHUB_TEMPLATES_VARIADIC_TEMPLATES_HPP
