# Templates Cheat Sheet

**Location:** `CppVerseHub/docs/cheat_sheets/Templates_CheatSheet.md`

## 📚 Complete C++ Templates Reference

### 🎯 Template Basics

#### **Function Templates**

```cpp
// Basic function template
template<typename T>
T max_value(T a, T b) {
    return (a > b) ? a : b;
}

// Multiple template parameters
template<typename T, typename U>
auto add(T a, U b) -> decltype(a + b) {
    return a + b;
}

// Template with non-type parameters
template<typename T, size_t N>
T sum_array(const T (&arr)[N]) {
    T total = T{};
    for (size_t i = 0; i < N; ++i) {
        total += arr[i];
    }
    return total;
}

// Usage examples
int main() {
    int x = 10, y = 20;
    double a = 3.14, b = 2.71;

    auto max_int = max_value(x, y);        // T deduced as int
    auto max_double = max_value(a, b);     // T deduced as double

    auto sum = add(x, a);                  // Mixed types

    int arr[] = {1, 2, 3, 4, 5};
    auto array_sum = sum_array(arr);       // N deduced as 5

    return 0;
}
```

#### **Class Templates**

```cpp
// Basic class template
template<typename T>
class Vector3D {
private:
    T x_, y_, z_;

public:
    Vector3D(T x = T{}, T y = T{}, T z = T{})
        : x_(x), y_(y), z_(z) {}

    T length() const {
        return std::sqrt(x_*x_ + y_*y_ + z_*z_);
    }

    Vector3D operator+(const Vector3D& other) const {
        return Vector3D(x_ + other.x_, y_ + other.y_, z_ + other.z_);
    }

    // Template member function
    template<typename U>
    Vector3D<U> convert() const {
        return Vector3D<U>(static_cast<U>(x_),
                          static_cast<U>(y_),
                          static_cast<U>(z_));
    }
};

// Multiple template parameters
template<typename T, size_t Dimensions>
class NDimensionalVector {
private:
    std::array<T, Dimensions> data_;

public:
    NDimensionalVector() { data_.fill(T{}); }

    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }

    static constexpr size_t dimensions() { return Dimensions; }

    T magnitude() const {
        T sum = T{};
        for (const auto& component : data_) {
            sum += component * component;
        }
        return std::sqrt(sum);
    }
};

// Usage
Vector3D<double> position(10.0, 20.0, 30.0);
Vector3D<int> grid_pos = position.convert<int>();

NDimensionalVector<float, 4> quaternion;
quaternion[0] = 1.0f;  // w component
```

#### **Variable Templates (C++14)**

```cpp
// Mathematical constants
template<typename T>
constexpr T pi = T(3.1415926535897932385);

template<typename T>
constexpr T e = T(2.7182818284590452354);

// Usage
double circle_area = pi<double> * radius * radius;
float natural_log = std::log(e<float>);

// Variable template for type traits
template<typename T>
constexpr bool is_space_entity_v = std::is_base_of_v<Entity, T>;

// Usage
static_assert(is_space_entity_v<Planet>);   // Compile-time check
```

### 🔧 Template Specialization

#### **Full Specialization**

```cpp
// Primary template
template<typename T>
class DataProcessor {
public:
    void process(const T& data) {
        std::cout << "Processing generic data\n";
    }
};

// Full specialization for std::string
template<>
class DataProcessor<std::string> {
public:
    void process(const std::string& data) {
        std::cout << "Processing string: " << data << "\n";
    }
};

// Full specialization for pointers
template<typename T>
class DataProcessor<T*> {
public:
    void process(T* const data) {
        if (data) {
            std::cout << "Processing pointer to data\n";
        }
    }
};

// Function template specialization
template<typename T>
void print_type() {
    std::cout << "Unknown type\n";
}

template<>
void print_type<int>() {
    std::cout << "Integer type\n";
}

template<>
void print_type<std::string>() {
    std::cout << "String type\n";
}
```

#### **Partial Specialization** (Class templates only)

```cpp
// Primary template
template<typename T, typename U>
class Pair {
private:
    T first_;
    U second_;

public:
    Pair(const T& first, const U& second)
        : first_(first), second_(second) {}

    void print() const {
        std::cout << "Generic pair\n";
    }
};

// Partial specialization - both types are the same
template<typename T>
class Pair<T, T> {
private:
    T first_;
    T second_;

public:
    Pair(const T& first, const T& second)
        : first_(first), second_(second) {}

    void print() const {
        std::cout << "Homogeneous pair\n";
    }

    bool are_equal() const {
        return first_ == second_;
    }
};

// Partial specialization - second type is pointer
template<typename T, typename U>
class Pair<T, U*> {
private:
    T first_;
    U* second_;

public:
    Pair(const T& first, U* second)
        : first_(first), second_(second) {}

    void print() const {
        std::cout << "Pair with pointer\n";
    }
};

// Usage
Pair<int, double> generic_pair(42, 3.14);        // Uses primary
Pair<int, int> same_type_pair(10, 20);           // Uses first specialization
Pair<int, std::string*> pointer_pair(5, &str);   // Uses second specialization
```

### 🧮 Template Metaprogramming

#### **Type Traits and SFINAE**

```cpp
#include <type_traits>

// Custom type trait
template<typename T>
struct is_planet : std::false_type {};

template<>
struct is_planet<Planet> : std::true_type {};

// Helper variable template
template<typename T>
constexpr bool is_planet_v = is_planet<T>::value;

// SFINAE with enable_if
template<typename T>
typename std::enable_if_t<std::is_arithmetic_v<T>, T>
safe_divide(T a, T b) {
    if (b != T{}) {
        return a / b;
    }
    throw std::invalid_argument("Division by zero");
}

// SFINAE with concepts-like approach using void_t
template<typename, typename = std::void_t<>>
struct has_update_method : std::false_type {};

template<typename T>
struct has_update_method<T, std::void_t<decltype(std::declval<T>().update())>>
    : std::true_type {};

template<typename T>
constexpr bool has_update_method_v = has_update_method<T>::value;

// Function that works only for types with update() method
template<typename T>
std::enable_if_t<has_update_method_v<T>>
update_if_possible(T& obj) {
    obj.update();
}

// Tag dispatching
struct arithmetic_tag {};
struct string_tag {};
struct other_tag {};

template<typename T>
using tag_type = std::conditional_t<
    std::is_arithmetic_v<T>, arithmetic_tag,
    std::conditional_t<
        std::is_same_v<T, std::string>, string_tag,
        other_tag>>;

template<typename T>
void process_data_impl(const T& data, arithmetic_tag) {
    std::cout << "Processing number: " << data << "\n";
}

template<typename T>
void process_data_impl(const T& data, string_tag) {
    std::cout << "Processing string: " << data << "\n";
}

template<typename T>
void process_data_impl(const T& data, other_tag) {
    std::cout << "Processing unknown type\n";
}

template<typename T>
void process_data(const T& data) {
    process_data_impl(data, tag_type<T>{});
}
```

#### **Compile-Time Computations**

```cpp
// Factorial at compile time
template<int N>
constexpr int factorial() {
    if constexpr (N <= 1) {
        return 1;
    } else {
        return N * factorial<N-1>();
    }
}

// Fibonacci sequence
template<int N>
constexpr int fibonacci() {
    if constexpr (N <= 1) {
        return N;
    } else {
        return fibonacci<N-1>() + fibonacci<N-2>();
    }
}

// Compile-time string hashing
constexpr std::size_t hash_string(const char* str) {
    std::size_t hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) + (*str++);
    }
    return hash;
}

// Usage at compile time
constexpr int fact_5 = factorial<5>();        // Computed at compile time
constexpr int fib_10 = fibonacci<10>();       // Computed at compile time
constexpr auto str_hash = hash_string("test"); // Computed at compile time

// Compile-time type list
template<typename... Types>
struct type_list {
    static constexpr size_t size = sizeof...(Types);
};

template<size_t Index, typename... Types>
struct type_at;

template<typename Head, typename... Tail>
struct type_at<0, Head, Tail...> {
    using type = Head;
};

template<size_t Index, typename Head, typename... Tail>
struct type_at<Index, Head, Tail...> {
    using type = typename type_at<Index - 1, Tail...>::type;
};

template<size_t Index, typename... Types>
using type_at_t = typename type_at<Index, Types...>::type;

// Usage
using my_types = type_list<int, double, std::string, Planet>;
using second_type = type_at_t<1, int, double, std::string>;  // double
```

### 📦 Variadic Templates

#### **Parameter Packs**

```cpp
// Basic variadic function template
template<typename... Args>
void print_args(Args... args) {
    ((std::cout << args << " "), ...);  // C++17 fold expression
    std::cout << "\n";
}

// Pre-C++17 recursive approach
template<typename T>
void print_recursive(T&& t) {
    std::cout << t << "\n";
}

template<typename T, typename... Args>
void print_recursive(T&& t, Args&&... args) {
    std::cout << t << " ";
    print_recursive(args...);
}

// Perfect forwarding with variadic templates
template<typename T, typename... Args>
std::unique_ptr<T> make_unique_custom(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// Variadic class template
template<typename... Types>
class MultiContainer {
private:
    std::tuple<std::vector<Types>...> containers_;

public:
    template<typename T>
    void add(const T& item) {
        std::get<std::vector<T>>(containers_).push_back(item);
    }

    template<typename T>
    const std::vector<T>& get() const {
        return std::get<std::vector<T>>(containers_);
    }

    template<typename Func>
    void for_each_container(Func func) {
        std::apply([&func](auto&... containers) {
            (func(containers), ...);  // C++17 fold expression
        }, containers_);
    }
};

// Usage
MultiContainer<int, std::string, double> multi;
multi.add(42);
multi.add(std::string("hello"));
multi.add(3.14);

multi.for_each_container([](const auto& container) {
    std::cout << "Container size: " << container.size() << "\n";
});
```

#### **Fold Expressions (C++17)**

```cpp
// Sum all arguments
template<typename... Args>
auto sum(Args... args) {
    return (args + ...);  // Unary right fold
}

// Print all arguments
template<typename... Args>
void print_all(Args... args) {
    ((std::cout << args << " "), ...);  // Binary fold with comma operator
}

// Check if all values are positive
template<typename... Args>
bool all_positive(Args... args) {
    return ((args > 0) && ...);  // Logical AND fold
}

// Find maximum value
template<typename T, typename... Args>
constexpr T max_value(T first, Args... args) {
    if constexpr (sizeof...(args) == 0) {
        return first;
    } else {
        return std::max(first, max_value(args...));
    }
}

// Alternative using fold expression
template<typename... Args>
auto max_fold(Args... args) {
    return std::max({args...});  // Using initializer list
}

// Usage
auto total = sum(1, 2, 3, 4, 5);              // 15
print_all("Hello", " ", "World", "!");        // Hello World!
bool positive = all_positive(1, 2, 3, 4);     // true
auto maximum = max_value(10, 5, 8, 15, 3);    // 15
```

### 🎯 C++20 Concepts

#### **Basic Concepts**

```cpp
#include <concepts>

// Simple concept
template<typename T>
concept Numeric = std::is_arithmetic_v<T>;

// Function using concept
template<Numeric T>
T square(T value) {
    return value * value;
}

// Custom concept with requirements
template<typename T>
concept Drawable = requires(T obj) {
    obj.draw();
    obj.move(0, 0);
    { obj.getPosition() } -> std::convertible_to<std::pair<int, int>>;
};

// Concept for container types
template<typename T>
concept Container = requires(T container) {
    container.begin();
    container.end();
    container.size();
    typename T::value_type;
    typename T::iterator;
};

// Entity concept for space simulation
template<typename T>
concept SpaceEntity = requires(T entity) {
    { entity.getId() } -> std::convertible_to<std::string>;
    { entity.getPosition() } -> std::convertible_to<Vector3D<double>>;
    entity.update();
} && std::is_base_of_v<Entity, T>;

// Usage with concepts
template<SpaceEntity T>
void simulate_entity(T& entity) {
    entity.update();
    auto pos = entity.getPosition();
    std::cout << "Entity " << entity.getId()
              << " at position " << pos << "\n";
}

// Concept composition
template<typename T>
concept SpaceEntityContainer = Container<T> &&
    SpaceEntity<typename T::value_type>;

// Function using composed concept
template<SpaceEntityContainer C>
void update_all_entities(C& container) {
    for (auto& entity : container) {
        entity.update();
    }
}
```

#### **Advanced Concepts**

```cpp
// Concept with multiple type parameters
template<typename T, typename U>
concept Comparable = requires(T a, U b) {
    { a < b } -> std::convertible_to<bool>;
    { a > b } -> std::convertible_to<bool>;
    { a == b } -> std::convertible_to<bool>;
};

// Concept subsumption
template<typename T>
concept Integral = std::is_integral_v<T>;

template<typename T>
concept SignedIntegral = Integral<T> && std::is_signed_v<T>;

// More specific concept is chosen
template<Integral T>
void process(T value) {
    std::cout << "Processing integral value\n";
}

template<SignedIntegral T>  // More constrained, preferred
void process(T value) {
    std::cout << "Processing signed integral value\n";
}

// Concept with nested requirements
template<typename T>
concept Range = requires(T range) {
    range.begin();
    range.end();
    requires std::input_iterator<decltype(range.begin())>;
    requires std::sentinel_for<decltype(range.end()), decltype(range.begin())>;
};

// Concept requiring specific member functions
template<typename T>
concept Serializable = requires(T obj, std::ostream& os, std::istream& is) {
    obj.serialize(os);
    obj.deserialize(is);
    { obj.getVersion() } -> std::convertible_to<int>;
};

// Factory concept
template<typename F, typename... Args>
concept Factory = requires(F factory, Args... args) {
    { factory.create(args...) } -> std::convertible_to<std::unique_ptr<typename F::product_type>>;
    typename F::product_type;
};
```

### 🏗️ Template Design Patterns

#### **CRTP (Curiously Recurring Template Pattern)**

```cpp
// Base class template taking derived class as parameter
template<typename Derived>
class Entity {
public:
    void update() {
        static_cast<Derived*>(this)->update_impl();
    }

    void render() const {
        static_cast<const Derived*>(this)->render_impl();
    }

    // Static polymorphism
    Derived& derived() { return static_cast<Derived&>(*this); }
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

class Planet : public Entity<Planet> {
public:
    void update_impl() {
        // Planet-specific update logic
        generateResources();
    }

    void render_impl() const {
        // Planet-specific rendering
        drawPlanet();
    }

private:
    void generateResources() { /* ... */ }
    void drawPlanet() const { /* ... */ }
};

// Template function working with CRTP base
template<typename T>
void process_entity(Entity<T>& entity) {
    entity.update();    // Calls derived implementation
    entity.render();    // Calls derived implementation
}
```

#### **Policy-Based Design**

```cpp
// Storage policies
class VectorStorage {
public:
    template<typename T>
    using container_type = std::vector<T>;
};

class ListStorage {
public:
    template<typename T>
    using container_type = std::list<T>;
};

// Threading policies
class SingleThreaded {
public:
    class Lock {
    public:
        Lock() {}  // No-op for single-threaded
    };
};

class MultiThreaded {
public:
    class Lock {
    private:
        std::mutex& mutex_;
    public:
        Lock(std::mutex& m) : mutex_(m) { mutex_.lock(); }
        ~Lock() { mutex_.unlock(); }
    };

private:
    static std::mutex mutex_;
};

// Template class using policies
template<typename T,
         template<typename> class StoragePolicy = VectorStorage,
         class ThreadingPolicy = SingleThreaded>
class DataManager : private ThreadingPolicy {
private:
    using container_type = typename StoragePolicy<T>::container_type;
    container_type data_;

public:
    void add(const T& item) {
        typename ThreadingPolicy::Lock lock;  // Thread-safe if needed
        data_.push_back(item);
    }

    T get(size_t index) {
        typename ThreadingPolicy::Lock lock;
        return data_[index];
    }

    size_t size() const {
        return data_.size();
    }
};

// Usage with different policies
DataManager<Planet> single_threaded_planets;  // Default: vector, single-threaded
DataManager<Fleet, ListStorage, MultiThreaded> thread_safe_fleets;
```

#### **Expression Templates**

```cpp
// Basic expression template for vector operations
template<typename T>
class Vector {
private:
    std::vector<T> data_;

public:
    Vector(size_t size) : data_(size) {}
    Vector(std::initializer_list<T> init) : data_(init) {}

    T& operator[](size_t i) { return data_[i]; }
    const T& operator[](size_t i) const { return data_[i]; }
    size_t size() const { return data_.size(); }

    // Assignment from expression
    template<typename Expr>
    Vector& operator=(const Expr& expr) {
        for (size_t i = 0; i < data_.size(); ++i) {
            data_[i] = expr[i];
        }
        return *this;
    }
};

// Expression template for addition
template<typename Left, typename Right>
class VectorAdd {
private:
    const Left& left_;
    const Right& right_;

public:
    VectorAdd(const Left& left, const Right& right)
        : left_(left), right_(right) {}

    auto operator[](size_t i) const -> decltype(left_[i] + right_[i]) {
        return left_[i] + right_[i];
    }

    size_t size() const { return left_.size(); }
};

// Expression template for scalar multiplication
template<typename Expr, typename Scalar>
class VectorScale {
private:
    const Expr& expr_;
    Scalar scalar_;

public:
    VectorScale(const Expr& expr, Scalar scalar)
        : expr_(expr), scalar_(scalar) {}

    auto operator[](size_t i) const -> decltype(expr_[i] * scalar_) {
        return expr_[i] * scalar_;
    }

    size_t size() const { return expr_.size(); }
};

// Operator overloads
template<typename T>
VectorAdd<Vector<T>, Vector<T>>
operator+(const Vector<T>& left, const Vector<T>& right) {
    return VectorAdd<Vector<T>, Vector<T>>(left, right);
}

template<typename Left, typename Right>
VectorAdd<Left, Right>
operator+(const Left& left, const Right& right) {
    return VectorAdd<Left, Right>(left, right);
}

template<typename T, typename Scalar>
VectorScale<Vector<T>, Scalar>
operator*(const Vector<T>& vec, Scalar scalar) {
    return VectorScale<Vector<T>, Scalar>(vec, scalar);
}

// Usage - no temporaries created!
Vector<double> a{1.0, 2.0, 3.0};
Vector<double> b{4.0, 5.0, 6.0};
Vector<double> c{7.0, 8.0, 9.0};
Vector<double> result(3);

result = a + b * 2.0 + c;  // Evaluated lazily, no intermediate vectors
```

### 🛠️ Template Best Practices

#### **Template Parameter Guidelines**

```cpp
// Use typename for dependent types
template<typename T>
class Container {
public:
    using value_type = T;
    using size_type = typename std::vector<T>::size_type;  // typename needed
    using iterator = typename std::vector<T>::iterator;    // typename needed
};

// Prefer class template argument deduction (CTAD) C++17
template<typename T>
class Wrapper {
private:
    T value_;
public:
    Wrapper(T value) : value_(value) {}
};

// Deduction guide (optional in this case)
template<typename T>
Wrapper(T) -> Wrapper<T>;

// Usage
Wrapper w(42);        // Type deduced as Wrapper<int>
Wrapper s("hello");   // Type deduced as Wrapper<const char*>

// Default template arguments
template<typename T, typename Allocator = std::allocator<T>>
class CustomVector {
    // Implementation using Allocator
};

// Non-type template parameters with auto (C++17)
template<auto Value>
class ValueHolder {
private:
    static constexpr auto value_ = Value;
public:
    static constexpr auto get() { return value_; }
};

ValueHolder<42> int_holder;        // Value is int
ValueHolder<3.14> double_holder;   // Value is double
```

#### **Error Handling and Debugging**

```cpp
// Static assertions for template constraints
template<typename T>
class NumericProcessor {
    static_assert(std::is_arithmetic_v<T>,
                  "T must be an arithmetic type");
    static_assert(sizeof(T) >= 4,
                  "T must be at least 4 bytes");
public:
    void process(T value) {
        // Processing logic
    }
};

// SFINAE-friendly detection
template<typename T, typename = void>
struct has_serialize : std::false_type {};

template<typename T>
struct has_serialize<T, std::void_t<
    decltype(std::declval<T>().serialize(std::declval<std::ostream&>()))
>> : std::true_type {};

// Template debugging with type printing
template<typename T>
struct TypePrinter;  // Intentionally undefined

// Usage: TypePrinter<T> printer; causes compile error showing T
// #define DEBUG_TYPE(T) TypePrinter<T> printer##__LINE__;

// Better error messages with concepts
template<typename T>
concept SerializableEntity = requires(T obj, std::ostream& os) {
    obj.serialize(os);
    obj.getId();
} && std::is_base_of_v<Entity, T>;

template<SerializableEntity T>
void save_entity(const T& entity, std::ostream& os) {
    entity.serialize(os);
}
```

### 📊 Template Performance Considerations

#### **Compile-Time vs Runtime Performance**

```cpp
// Compile-time computation
template<int N>
constexpr int power_of_2() {
    return 1 << N;
}

constexpr int result = power_of_2<10>();  // Computed at compile time

// Template instantiation control
extern template class std::vector<int>;  // Prevent instantiation
extern template class std::vector<double>;

// Explicit instantiation (in .cpp file)
template class std::vector<int>;         // Force instantiation

// Template specialization for performance
template<typename T>
void sort_data(std::vector<T>& data) {
    std::sort(data.begin(), data.end());
}

// Specialization for simple types
template<>
void sort_data<int>(std::vector<int>& data) {
    std::sort(data.begin(), data.end(), std::less<int>());  // Optimized
}

// Template metaprogramming for optimization
template<size_t N>
struct UnrollLoop {
    template<typename Func>
    static void execute(Func func) {
        func(N-1);
        UnrollLoop<N-1>::execute(func);
    }
};

template<>
struct UnrollLoop<0> {
    template<typename Func>
    static void execute(Func func) {}  // Base case
};

// Usage: loop unrolled at compile time
UnrollLoop<5>::execute([](size_t i) {
    std::cout << "Iteration " << i << "\n";
});
```

### 🎯 Key Template Takeaways

1. **Start simple** - Basic function and class templates first
2. **Use concepts** (C++20) for better error messages and constraints
3. **Prefer auto** and template argument deduction when possible
4. **SFINAE** is powerful but concepts are cleaner
5. **Template specialization** enables optimization for specific types
6. **Variadic templates** handle varying numbers of arguments elegantly
7. **Compile-time computations** can improve runtime performance
8. **Template metaprogramming** is powerful but use judiciously
9. **Expression templates** can eliminate temporary objects
10. **Policy-based design** provides flexibility without virtual function overhead

### 📋 Template Quick Reference

| Feature                 | Syntax                                 | Use Case                  |
| ----------------------- | -------------------------------------- | ------------------------- |
| Function Template       | `template<typename T>`                 | Generic functions         |
| Class Template          | `template<typename T> class C`         | Generic classes           |
| Variable Template       | `template<typename T> constexpr T var` | Generic constants         |
| Concepts                | `template<Concept T>`                  | Type constraints          |
| SFINAE                  | `enable_if_t<condition>`               | Conditional compilation   |
| Fold Expressions        | `(args + ...)`                         | Variadic operations       |
| Perfect Forwarding      | `T&& t, forward<T>(t)`                 | Preserve value categories |
| CRTP                    | `Base<Derived>`                        | Static polymorphism       |
| Template Specialization | `template<> class C<int>`              | Type-specific behavior    |

---

_This cheat sheet covers essential template programming concepts used throughout CppVerseHub. For practical examples, see the `src/templates/` directory._
