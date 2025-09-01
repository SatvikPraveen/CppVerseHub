# Exception Handling Cheat Sheet

**Location:** `CppVerseHub/docs/cheat_sheets/Exceptions_CheatSheet.md`

## 📚 C++ Exception Handling Best Practices

### 🎯 Exception Basics

#### **Exception Syntax**

```cpp
#include <stdexcept>
#include <iostream>

// Basic try-catch structure
void basic_exception_handling() {
    try {
        // Code that might throw
        throw std::runtime_error("Something went wrong!");

    } catch (const std::runtime_error& e) {
        std::cout << "Caught runtime_error: " << e.what() << "\n";

    } catch (const std::exception& e) {
        std::cout << "Caught exception: " << e.what() << "\n";

    } catch (...) {
        std::cout << "Caught unknown exception\n";
    }
}

// Multiple catch blocks (most specific first)
void multiple_catch_blocks() {
    try {
        // Some operation that might throw various exceptions
        int choice = 2;

        if (choice == 1) {
            throw std::invalid_argument("Invalid argument provided");
        } else if (choice == 2) {
            throw std::runtime_error("Runtime error occurred");
        } else {
            throw std::logic_error("Logic error detected");
        }

    } catch (const std::invalid_argument& e) {
        std::cout << "Invalid argument: " << e.what() << "\n";

    } catch (const std::logic_error& e) {
        std::cout << "Logic error: " << e.what() << "\n";

    } catch (const std::runtime_error& e) {
        std::cout << "Runtime error: " << e.what() << "\n";

    } catch (const std::exception& e) {
        std::cout << "General exception: " << e.what() << "\n";
    }
}

// Re-throwing exceptions
void rethrow_example() {
    try {
        try {
            throw std::runtime_error("Original error");

        } catch (const std::exception& e) {
            std::cout << "Inner catch: " << e.what() << "\n";

            // Add context and re-throw
            throw std::runtime_error("Enhanced error: " + std::string(e.what()));
        }

    } catch (const std::exception& e) {
        std::cout << "Outer catch: " << e.what() << "\n";
    }
}
```

#### **Standard Exception Hierarchy**

```cpp
#include <stdexcept>

// Standard exception types demonstration
class ExceptionDemo {
public:
    void demonstrate_standard_exceptions() {
        std::cout << "=== Standard Exception Types ===\n";

        try {
            throw_logic_error();
        } catch (const std::exception& e) {
            std::cout << "Logic error: " << e.what() << "\n";
        }

        try {
            throw_runtime_error();
        } catch (const std::exception& e) {
            std::cout << "Runtime error: " << e.what() << "\n";
        }

        try {
            throw_system_error();
        } catch (const std::exception& e) {
            std::cout << "System error: " << e.what() << "\n";
        }
    }

private:
    void throw_logic_error() {
        // Logic errors - programming mistakes

        // std::invalid_argument - invalid parameter
        throw std::invalid_argument("Planet name cannot be empty");

        // std::domain_error - domain error in math functions
        // throw std::domain_error("Square root of negative number");

        // std::length_error - length exceeds maximum allowed
        // throw std::length_error("Fleet size exceeds maximum capacity");

        // std::out_of_range - access outside valid range
        // throw std::out_of_range("Planet index out of range");
    }

    void throw_runtime_error() {
        // Runtime errors - external factors

        // std::runtime_error - general runtime error
        throw std::runtime_error("Failed to connect to mission control");

        // std::range_error - result outside valid range
        // throw std::range_error("Calculated trajectory out of valid range");

        // std::overflow_error - arithmetic overflow
        // throw std::overflow_error("Resource calculation overflow");

        // std::underflow_error - arithmetic underflow
        // throw std::underflow_error("Energy calculation underflow");
    }

    void throw_system_error() {
        // System errors - OS level errors
        throw std::system_error(
            std::error_code(ENOENT, std::system_category()),
            "Configuration file not found"
        );
    }
};
```

### 🏗️ Custom Exception Classes

#### **Basic Custom Exceptions**

```cpp
// Base custom exception class
class SpaceSimulationError : public std::exception {
private:
    std::string message_;
    std::string component_;
    int error_code_;

public:
    SpaceSimulationError(const std::string& component,
                        const std::string& message,
                        int code = 0)
        : component_(component), message_(message), error_code_(code) {

        // Build full error message
        full_message_ = "[" + component_ + "] " + message_;
        if (error_code_ != 0) {
            full_message_ += " (Code: " + std::to_string(error_code_) + ")";
        }
    }

    const char* what() const noexcept override {
        return full_message_.c_str();
    }

    const std::string& getComponent() const { return component_; }
    const std::string& getMessage() const { return message_; }
    int getErrorCode() const { return error_code_; }

private:
    std::string full_message_;
};

// Specific exception types
class MissionError : public SpaceSimulationError {
public:
    enum class Type {
        InvalidTarget,
        InsufficientResources,
        FleetUnavailable,
        MissionConflict
    };

private:
    Type error_type_;

public:
    MissionError(Type type, const std::string& details)
        : SpaceSimulationError("Mission", buildMessage(type, details), static_cast<int>(type))
        , error_type_(type) {}

    Type getType() const { return error_type_; }

private:
    std::string buildMessage(Type type, const std::string& details) {
        std::string base_message;

        switch (type) {
            case Type::InvalidTarget:
                base_message = "Invalid mission target: ";
                break;
            case Type::InsufficientResources:
                base_message = "Insufficient resources for mission: ";
                break;
            case Type::FleetUnavailable:
                base_message = "Fleet unavailable for mission: ";
                break;
            case Type::MissionConflict:
                base_message = "Mission conflicts with existing operations: ";
                break;
        }

        return base_message + details;
    }
};

class ResourceError : public SpaceSimulationError {
public:
    enum class Type {
        Depleted,
        InvalidType,
        AllocationFailed,
        AccessDenied
    };

private:
    Type error_type_;
    double requested_amount_;
    double available_amount_;

public:
    ResourceError(Type type, const std::string& resource_name,
                 double requested = 0, double available = 0)
        : SpaceSimulationError("Resource", buildMessage(type, resource_name, requested, available))
        , error_type_(type), requested_amount_(requested), available_amount_(available) {}

    Type getType() const { return error_type_; }
    double getRequestedAmount() const { return requested_amount_; }
    double getAvailableAmount() const { return available_amount_; }

private:
    std::string buildMessage(Type type, const std::string& resource_name,
                           double requested, double available) {
        std::string message;

        switch (type) {
            case Type::Depleted:
                message = "Resource '" + resource_name + "' depleted";
                if (requested > 0 && available >= 0) {
                    message += " (requested: " + std::to_string(requested) +
                              ", available: " + std::to_string(available) + ")";
                }
                break;

            case Type::InvalidType:
                message = "Invalid resource type: " + resource_name;
                break;

            case Type::AllocationFailed:
                message = "Failed to allocate resource: " + resource_name;
                break;

            case Type::AccessDenied:
                message = "Access denied to resource: " + resource_name;
                break;
        }

        return message;
    }
};

// Usage of custom exceptions
void custom_exception_examples() {
    try {
        // Simulate mission planning
        std::string target_planet = "";

        if (target_planet.empty()) {
            throw MissionError(MissionError::Type::InvalidTarget, "Target planet not specified");
        }

        // Simulate resource allocation
        double available_fuel = 100.0;
        double required_fuel = 150.0;

        if (required_fuel > available_fuel) {
            throw ResourceError(ResourceError::Type::Depleted, "Fuel",
                              required_fuel, available_fuel);
        }

    } catch (const MissionError& e) {
        std::cout << "Mission Error (" << static_cast<int>(e.getType()) << "): "
                  << e.what() << "\n";

        // Handle specific mission error types
        switch (e.getType()) {
            case MissionError::Type::InvalidTarget:
                std::cout << "  -> Requesting user to specify target\n";
                break;
            case MissionError::Type::InsufficientResources:
                std::cout << "  -> Attempting to acquire additional resources\n";
                break;
            default:
                std::cout << "  -> Generic mission error handling\n";
                break;
        }

    } catch (const ResourceError& e) {
        std::cout << "Resource Error: " << e.what() << "\n";

        if (e.getType() == ResourceError::Type::Depleted) {
            std::cout << "  -> Need " << (e.getRequestedAmount() - e.getAvailableAmount())
                      << " more units\n";
        }

    } catch (const SpaceSimulationError& e) {
        std::cout << "Simulation Error [" << e.getComponent() << "]: "
                  << e.getMessage() << "\n";

    } catch (const std::exception& e) {
        std::cout << "Unexpected error: " << e.what() << "\n";
    }
}
```

#### **Exception with Stack Trace**

```cpp
#include <vector>
#include <sstream>

// Exception class with stack trace capability
class DetailedError : public std::exception {
private:
    std::string message_;
    std::vector<std::string> stack_trace_;
    mutable std::string full_what_;

public:
    DetailedError(const std::string& message) : message_(message) {
        capture_stack_trace();
        build_what_string();
    }

    const char* what() const noexcept override {
        return full_what_.c_str();
    }

    const std::vector<std::string>& getStackTrace() const {
        return stack_trace_;
    }

    void addStackFrame(const std::string& function, const std::string& file, int line) {
        std::string frame = function + " (" + file + ":" + std::to_string(line) + ")";
        stack_trace_.push_back(frame);
        build_what_string();
    }

private:
    void capture_stack_trace() {
        // Simplified stack trace capture
        // In real implementation, you might use platform-specific APIs
        // like backtrace() on Unix or CaptureStackBackTrace() on Windows

        stack_trace_.push_back("DetailedError constructor");
    }

    void build_what_string() const {
        std::ostringstream oss;
        oss << "Error: " << message_ << "\n";
        oss << "Stack trace:\n";

        for (size_t i = 0; i < stack_trace_.size(); ++i) {
            oss << "  " << i << ": " << stack_trace_[i] << "\n";
        }

        full_what_ = oss.str();
    }
};

// Macro for easy stack frame addition
#define THROW_DETAILED_ERROR(msg) do { \
    DetailedError e(msg); \
    e.addStackFrame(__FUNCTION__, __FILE__, __LINE__); \
    throw e; \
} while(0)

// Example usage
void deep_function() {
    THROW_DETAILED_ERROR("Deep function error");
}

void middle_function() {
    try {
        deep_function();
    } catch (DetailedError& e) {
        e.addStackFrame(__FUNCTION__, __FILE__, __LINE__);
        throw;  // Re-throw with additional stack info
    }
}

void top_function() {
    try {
        middle_function();
    } catch (const DetailedError& e) {
        std::cout << e.what() << "\n";
    }
}
```

### 🛡️ Exception Safety Guarantees

#### **Exception Safety Levels**

```cpp
#include <memory>

class Planet {
private:
    std::string name_;
    double resources_;

public:
    Planet(const std::string& name, double resources)
        : name_(name), resources_(resources) {}

    const std::string& getName() const { return name_; }
    double getResources() const { return resources_; }
    void setResources(double resources) { resources_ = resources; }
};

// 1. No-throw guarantee (nothrow)
class NoThrowExample {
private:
    std::vector<std::unique_ptr<Planet>> planets_;

public:
    // Functions that never throw
    size_t getPlanetCount() const noexcept {
        return planets_.size();
    }

    bool isEmpty() const noexcept {
        return planets_.empty();
    }

    // Swap operation - typically no-throw
    void swap(NoThrowExample& other) noexcept {
        planets_.swap(other.planets_);
    }

    // Move operations should be no-throw when possible
    NoThrowExample(NoThrowExample&& other) noexcept
        : planets_(std::move(other.planets_)) {}

    NoThrowExample& operator=(NoThrowExample&& other) noexcept {
        if (this != &other) {
            planets_ = std::move(other.planets_);
        }
        return *this;
    }
};

// 2. Strong exception safety (commit-or-rollback)
class StrongSafetyExample {
private:
    std::vector<std::unique_ptr<Planet>> planets_;
    double total_resources_;

public:
    StrongSafetyExample() : total_resources_(0.0) {}

    // Strong guarantee - either succeeds completely or leaves object unchanged
    void addPlanet(const std::string& name, double resources) {
        // Create new planet first (may throw)
        auto new_planet = std::make_unique<Planet>(name, resources);

        // Calculate new total (may throw, but hasn't modified state yet)
        double new_total = total_resources_ + resources;

        // If we reach here, all operations that might throw have succeeded
        // Now perform no-throw operations to commit changes
        planets_.push_back(std::move(new_planet));
        total_resources_ = new_total;
    }

    // Strong guarantee using copy-and-swap idiom
    void replaceAllPlanets(const std::vector<std::pair<std::string, double>>& planet_data) {
        // Build new state in temporary object
        StrongSafetyExample temp;

        for (const auto& [name, resources] : planet_data) {
            temp.addPlanet(name, resources);  // May throw, but doesn't affect *this
        }

        // If we reach here, all operations succeeded
        // Atomically swap states (no-throw)
        swap(temp);
    }

    void swap(StrongSafetyExample& other) noexcept {
        planets_.swap(other.planets_);
        std::swap(total_resources_, other.total_resources_);
    }

    double getTotalResources() const noexcept {
        return total_resources_;
    }
};

// 3. Basic exception safety (no resource leaks)
class BasicSafetyExample {
private:
    std::vector<std::unique_ptr<Planet>> planets_;
    std::unique_ptr<ComplexResource> resource_;

public:
    // Basic guarantee - no resource leaks, object in valid state
    void riskyOperation() {
        // RAII ensures cleanup even if exceptions occur
        auto temp_resource = std::make_unique<ComplexResource>();

        // This might throw, but temp_resource will be cleaned up
        planets_.push_back(std::make_unique<Planet>("Risky Planet", 500.0));

        // This might also throw
        temp_resource->performComplexOperation();

        // If we reach here, commit the resource
        resource_ = std::move(temp_resource);
    }

    // Basic guarantee - might leave object in different but valid state
    void processAllPlanets() {
        for (auto& planet : planets_) {
            // Each planet operation might throw
            // Some planets might be processed, others might not
            // But no resource leaks occur due to RAII
            planet->setResources(planet->getResources() * 1.1);

            // If this throws, previous planets are still processed
            // Object is in valid but different state
        }
    }
};

// 4. No exception safety (undefined behavior if exception occurs)
class UnsafeExample {
private:
    Planet** planets_;  // Raw pointer array - BAD!
    size_t count_;
    size_t capacity_;

public:
    UnsafeExample() : planets_(nullptr), count_(0), capacity_(0) {}

    // UNSAFE - resource leaks and undefined behavior possible
    void unsafeAddPlanet(const std::string& name, double resources) {
        if (count_ == capacity_) {
            // Reallocate - DANGEROUS if exception occurs
            Planet** new_array = new Planet*[capacity_ * 2 + 1];

            // If this throws, new_array leaks and planets_ is unchanged
            for (size_t i = 0; i < count_; ++i) {
                new_array[i] = planets_[i];
            }

            delete[] planets_;  // What if this was already null?
            planets_ = new_array;
            capacity_ = capacity_ * 2 + 1;
        }

        // If this throws, we have inconsistent state
        planets_[count_] = new Planet(name, resources);  // Raw new - BAD!
        ++count_;
    }

    ~UnsafeExample() {
        // Cleanup - but what if count_ is inconsistent?
        for (size_t i = 0; i < count_; ++i) {
            delete planets_[i];
        }
        delete[] planets_;
    }
};
```

#### **RAII and Exception Safety**

```cpp
// RAII class for automatic resource management
template<typename Resource, typename Deleter>
class RAIIGuard {
private:
    Resource resource_;
    Deleter deleter_;
    bool owns_resource_;

public:
    RAIIGuard(Resource resource, Deleter deleter)
        : resource_(resource), deleter_(deleter), owns_resource_(true) {}

    ~RAIIGuard() {
        if (owns_resource_) {
            try {
                deleter_(resource_);
            } catch (...) {
                // Never throw from destructor
                // Log error in real implementation
            }
        }
    }

    // Non-copyable
    RAIIGuard(const RAIIGuard&) = delete;
    RAIIGuard& operator=(const RAIIGuard&) = delete;

    // Movable
    RAIIGuard(RAIIGuard&& other) noexcept
        : resource_(other.resource_), deleter_(std::move(other.deleter_)), owns_resource_(other.owns_resource_) {
        other.owns_resource_ = false;
    }

    Resource get() const { return resource_; }

    Resource release() {
        owns_resource_ = false;
        return resource_;
    }
};

// Exception-safe transaction pattern
class TransactionGuard {
private:
    std::vector<std::function<void()>> rollback_actions_;
    bool committed_;

public:
    TransactionGuard() : committed_(false) {}

    ~TransactionGuard() {
        if (!committed_) {
            rollback();
        }
    }

    template<typename Action, typename Rollback>
    void addAction(Action&& action, Rollback&& rollback) {
        // Execute action first
        action();

        // If action succeeded, add rollback to the front of the list
        // (rollback in reverse order)
        rollback_actions_.insert(rollback_actions_.begin(), rollback);
    }

    void commit() {
        committed_ = true;
        rollback_actions_.clear();
    }

private:
    void rollback() {
        for (auto& action : rollback_actions_) {
            try {
                action();
            } catch (...) {
                // Log error but continue rollback
            }
        }
    }
};

// Usage example
void exception_safe_operations() {
    std::vector<std::unique_ptr<Planet>> planets;
    double total_resources = 1000.0;

    TransactionGuard transaction;

    try {
        // Add several planets transactionally
        transaction.addAction(
            [&]() {
                planets.push_back(std::make_unique<Planet>("Earth", 500.0));
                total_resources -= 500.0;
            },
            [&]() {
                if (!planets.empty()) {
                    planets.pop_back();
                    total_resources += 500.0;
                }
            }
        );

        transaction.addAction(
            [&]() {
                planets.push_back(std::make_unique<Planet>("Mars", 300.0));
                total_resources -= 300.0;
            },
            [&]() {
                if (!planets.empty()) {
                    planets.pop_back();
                    total_resources += 300.0;
                }
            }
        );

        // Simulate potential failure
        if (total_resources < 0) {
            throw std::runtime_error("Insufficient resources");
        }

        // If we reach here, commit all changes
        transaction.commit();

    } catch (const std::exception& e) {
        std::cout << "Transaction failed: " << e.what() << "\n";
        // Rollback happens automatically in TransactionGuard destructor
    }
}
```

### 🔄 Exception Handling Patterns

#### **Exception Translation**

```cpp
// Translate between different exception types
class ExceptionTranslator {
public:
    // Translate system exceptions to application exceptions
    static void translateFileOperations(const std::function<void()>& operation) {
        try {
            operation();

        } catch (const std::ios_base::failure& e) {
            throw ResourceError(ResourceError::Type::AccessDenied, "File operation failed: " + std::string(e.what()));

        } catch (const std::bad_alloc& e) {
            throw ResourceError(ResourceError::Type::AllocationFailed, "Memory allocation failed: " + std::string(e.what()));

        } catch (const std::system_error& e) {
            throw SpaceSimulationError("System", "System error: " + std::string(e.what()), e.code().value());

        } catch (const std::exception& e) {
            throw SpaceSimulationError("Unknown", "Unexpected error: " + std::string(e.what()));
        }
    }

    // Translate third-party library exceptions
    template<typename ThirdPartyFunc>
    static auto translateThirdParty(ThirdPartyFunc&& func) -> decltype(func()) {
        try {
            return func();

        } catch (const ThirdPartyException& e) {
            throw SpaceSimulationError("ThirdParty", "Third party error: " + std::string(e.what()));

        } catch (...) {
            throw SpaceSimulationError("ThirdParty", "Unknown third party error");
        }
    }
};

// Exception boundary pattern
class ExceptionBoundary {
public:
    // Execute function and handle all exceptions
    template<typename Func>
    static bool safeExecute(Func&& func, std::string& error_message) noexcept {
        try {
            func();
            return true;

        } catch (const SpaceSimulationError& e) {
            error_message = "Simulation error: " + std::string(e.what());

        } catch (const std::exception& e) {
            error_message = "Standard error: " + std::string(e.what());

        } catch (...) {
            error_message = "Unknown error occurred";
        }

        return false;
    }

    // Execute and return optional result
    template<typename Func>
    static std::optional<typename std::result_of<Func()>::type>
    tryExecute(Func&& func) noexcept {

        try {
            if constexpr (std::is_void_v<typename std::result_of<Func()>::type>) {
                func();
                return std::make_optional(true);  // Dummy value for void functions
            } else {
                return func();
            }

        } catch (...) {
            return std::nullopt;
        }
    }
};

void exception_translation_examples() {
    // File operation with translation
    ExceptionTranslator::translateFileOperations([]() {
        std::ifstream file("nonexistent.txt");
        if (!file) {
            throw std::ios_base::failure("File not found");
        }
    });

    // Safe execution
    std::string error_msg;
    bool success = ExceptionBoundary::safeExecute([]() {
        throw std::runtime_error("Test error");
    }, error_msg);

    if (!success) {
        std::cout << "Operation failed: " << error_msg << "\n";
    }

    // Try execute with optional result
    auto result = ExceptionBoundary::tryExecute([]() -> int {
        return 42;
    });

    if (result) {
        std::cout << "Result: " << *result << "\n";
    } else {
        std::cout << "Operation failed\n";
    }
}
```

#### **Exception Chaining**

```cpp
// Exception that can hold cause information
class ChainedException : public std::exception {
private:
    std::string message_;
    std::unique_ptr<std::exception> cause_;
    mutable std::string full_message_;

public:
    ChainedException(const std::string& message)
        : message_(message), cause_(nullptr) {}

    ChainedException(const std::string& message, std::unique_ptr<std::exception> cause)
        : message_(message), cause_(std::move(cause)) {}

    const char* what() const noexcept override {
        if (full_message_.empty()) {
            full_message_ = message_;

            if (cause_) {
                full_message_ += "\nCaused by: ";
                full_message_ += cause_->what();
            }
        }

        return full_message_.c_str();
    }

    const std::exception* getCause() const noexcept {
        return cause_.get();
    }

    std::string getMessage() const {
        return message_;
    }
};

// Helper function to create chained exceptions
template<typename ExceptionType>
ChainedException chain_exception(const std::string& message, const ExceptionType& cause) {
    return ChainedException(message, std::make_unique<ExceptionType>(cause));
}

// Usage example
void exception_chaining_example() {
    try {
        try {
            try {
                throw std::runtime_error("Database connection failed");

            } catch (const std::runtime_error& e) {
                throw chain_exception("Failed to load planet data", e);
            }

        } catch (const ChainedException& e) {
            throw chain_exception("Mission initialization failed", e);
        }

    } catch (const ChainedException& e) {
        std::cout << "Final error: " << e.what() << "\n";

        // Walk the exception chain
        const std::exception* current = &e;
        int level = 0;

        while (current) {
            std::cout << "Level " << level << ": " << current->what() << "\n";

            if (auto chained = dynamic_cast<const ChainedException*>(current)) {
                current = chained->getCause();
            } else {
                break;
            }

            ++level;
        }
    }
}
```

#### **Exception Aggregation**

```cpp
// Exception that collects multiple errors
class AggregateException : public std::exception {
private:
    std::vector<std::unique_ptr<std::exception>> exceptions_;
    mutable std::string cached_message_;

public:
    AggregateException() = default;

    AggregateException(std::vector<std::unique_ptr<std::exception>> exceptions)
        : exceptions_(std::move(exceptions)) {}

    void addException(std::unique_ptr<std::exception> exception) {
        exceptions_.push_back(std::move(exception));
        cached_message_.clear();  // Invalidate cache
    }

    template<typename ExceptionType>
    void addException(const ExceptionType& exception) {
        exceptions_.push_back(std::make_unique<ExceptionType>(exception));
        cached_message_.clear();
    }

    const char* what() const noexcept override {
        if (cached_message_.empty()) {
            std::ostringstream oss;
            oss << "Multiple errors occurred (" << exceptions_.size() << " total):\n";

            for (size_t i = 0; i < exceptions_.size(); ++i) {
                oss << "  " << (i + 1) << ": " << exceptions_[i]->what() << "\n";
            }

            cached_message_ = oss.str();
        }

        return cached_message_.c_str();
    }

    size_t getExceptionCount() const noexcept {
        return exceptions_.size();
    }

    const std::exception& getException(size_t index) const {
        if (index >= exceptions_.size()) {
            throw std::out_of_range("Exception index out of range");
        }
        return *exceptions_[index];
    }

    bool empty() const noexcept {
        return exceptions_.empty();
    }
};

// Parallel operation exception collector
class ParallelOperationManager {
public:
    template<typename Func>
    static void executeParallel(const std::vector<Func>& operations) {
        AggregateException aggregate;
        std::mutex exception_mutex;

        std::vector<std::thread> threads;

        for (size_t i = 0; i < operations.size(); ++i) {
            threads.emplace_back([&, i]() {
                try {
                    operations[i]();

                } catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lock(exception_mutex);
                    aggregate.addException(e);

                } catch (...) {
                    std::lock_guard<std::mutex> lock(exception_mutex);
                    aggregate.addException(std::runtime_error("Unknown error in parallel operation " + std::to_string(i)));
                }
            });
        }

        // Wait for all threads
        for (auto& thread : threads) {
            thread.join();
        }

        // Throw aggregate if any errors occurred
        if (!aggregate.empty()) {
            throw aggregate;
        }
    }
};

void aggregate_exception_example() {
    try {
        std::vector<std::function<void()>> operations = {
            []() { throw std::runtime_error("Operation 1 failed"); },
            []() { /* Success */ },
            []() { throw std::logic_error("Operation 3 failed"); },
            []() { throw std::invalid_argument("Operation 4 failed"); }
        };

        ParallelOperationManager::executeParallel(operations);

    } catch (const AggregateException& e) {
        std::cout << e.what() << "\n";

        // Handle individual exceptions
        for (size_t i = 0; i < e.getExceptionCount(); ++i) {
            const auto& inner = e.getException(i);
            std::cout << "Handling exception " << (i + 1) << ": " << inner.what() << "\n";
        }
    }
}
```

### 🧪 Exception Testing and Debugging

#### **Exception Testing Utilities**

```cpp
#include <cassert>

// Exception testing helper
class ExceptionTester {
public:
    template<typename ExceptionType, typename Func>
    static void expectException(Func&& func, const std::string& expected_message = "") {
        bool caught_expected = false;

        try {
            func();

        } catch (const ExceptionType& e) {
            caught_expected = true;

            if (!expected_message.empty()) {
                std::string actual_message = e.what();
                if (actual_message.find(expected_message) == std::string::npos) {
                    throw std::runtime_error(
                        "Exception message mismatch. Expected: '" + expected_message +
                        "', Actual: '" + actual_message + "'"
                    );
                }
            }

            std::cout << "✓ Caught expected exception: " << e.what() << "\n";

        } catch (const std::exception& e) {
            throw std::runtime_error(
                "Caught unexpected exception type: " + std::string(e.what())
            );
        }

        if (!caught_expected) {
            throw std::runtime_error("Expected exception was not thrown");
        }
    }

    template<typename Func>
    static void expectNoException(Func&& func) {
        try {
            func();
            std::cout << "✓ No exception thrown as expected\n";

        } catch (const std::exception& e) {
            throw std::runtime_error(
                "Unexpected exception thrown: " + std::string(e.what())
            );
        }
    }
};

// Mock object for testing exception scenarios
class MockPlanet {
private:
    std::string name_;
    double resources_;
    bool should_throw_;

public:
    MockPlanet(const std::string& name, double resources, bool should_throw = false)
        : name_(name), resources_(resources), should_throw_(should_throw) {}

    const std::string& getName() const {
        if (should_throw_) {
            throw std::runtime_error("Mock exception in getName()");
        }
        return name_;
    }

    double getResources() const {
        if (should_throw_) {
            throw ResourceError(ResourceError::Type::AccessDenied, "Mock resource access denied");
        }
        return resources_;
    }

    void setThrowMode(bool should_throw) {
        should_throw_ = should_throw;
    }
};

void exception_testing_examples() {
    std::cout << "=== Exception Testing Examples ===\n";

    // Test that specific exception is thrown
    ExceptionTester::expectException<ResourceError>([]() {
        MockPlanet planet("TestPlanet", 100.0, true);
        planet.getResources();  // Should throw ResourceError
    }, "Mock resource access denied");

    // Test that no exception is thrown
    ExceptionTester::expectNoException([]() {
        MockPlanet planet("TestPlanet", 100.0, false);
        auto resources = planet.getResources();
        assert(resources == 100.0);
    });

    // Test exception handling in complex scenarios
    ExceptionTester::expectException<AggregateException>([]() {
        AggregateException aggregate;
        aggregate.addException(std::runtime_error("Test error 1"));
        aggregate.addException(std::logic_error("Test error 2"));
        throw aggregate;
    });

    std::cout << "All exception tests passed!\n";
}
```

#### **Exception Debugging Tools**

```cpp
#include <chrono>
#include <fstream>

// Exception logger for debugging
class ExceptionLogger {
private:
    static std::ofstream log_file_;
    static std::mutex log_mutex_;
    static bool enabled_;

public:
    static void enable(const std::string& log_filename = "exceptions.log") {
        std::lock_guard<std::mutex> lock(log_mutex_);

        if (log_file_.is_open()) {
            log_file_.close();
        }

        log_file_.open(log_filename, std::ios::app);
        enabled_ = log_file_.is_open();
    }

    static void disable() {
        std::lock_guard<std::mutex> lock(log_mutex_);

        if (log_file_.is_open()) {
            log_file_.close();
        }
        enabled_ = false;
    }

    static void logException(const std::exception& e, const std::string& context = "") {
        if (!enabled_) return;

        std::lock_guard<std::mutex> lock(log_mutex_);

        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        log_file_ << "=== Exception Log Entry ===\n";
        log_file_ << "Timestamp: " << std::ctime(&time_t);
        log_file_ << "Context: " << (context.empty() ? "Unknown" : context) << "\n";
        log_file_ << "Exception: " << e.what() << "\n";
        log_file_ << "Type: " << typeid(e).name() << "\n";

        // Add stack trace if available
        if (const auto* detailed = dynamic_cast<const DetailedError*>(&e)) {
            log_file_ << "Stack trace:\n";
            for (const auto& frame : detailed->getStackTrace()) {
                log_file_ << "  " << frame << "\n";
            }
        }

        log_file_ << "========================\n\n";
        log_file_.flush();
    }

    static void logExceptionChain(const ChainedException& e, const std::string& context = "") {
        if (!enabled_) return;

        std::lock_guard<std::mutex> lock(log_mutex_);

        log_file_ << "=== Chained Exception Log ===\n";
        log_file_ << "Context: " << context << "\n";

        const std::exception* current = &e;
        int level = 0;

        while (current) {
            log_file_ << "Level " << level << ": " << current->what() << " (" << typeid(*current).name() << ")\n";

            if (const auto* chained = dynamic_cast<const ChainedException*>(current)) {
                current = chained->getCause();
            } else {
                break;
            }

            ++level;
        }

        log_file_ << "===========================\n\n";
        log_file_.flush();
    }
};

// Static member definitions (would be in .cpp file)
std::ofstream ExceptionLogger::log_file_;
std::mutex ExceptionLogger::log_mutex_;
bool ExceptionLogger::enabled_ = false;

// Exception debugging macros
#define TRY_WITH_CONTEXT(context) \
    try {

#define CATCH_AND_LOG(context) \
    } catch (const std::exception& e) { \
        ExceptionLogger::logException(e, context); \
        throw; \
    }

#define SAFE_EXECUTE(context, code) do { \
    TRY_WITH_CONTEXT(context) \
        code \
    CATCH_AND_LOG(context) \
} while(0)

void exception_debugging_example() {
    ExceptionLogger::enable("debug_exceptions.log");

    // Example 1: Manual logging
    try {
        throw MissionError(MissionError::Type::InsufficientResources, "Not enough fuel");

    } catch (const std::exception& e) {
        ExceptionLogger::logException(e, "Mission planning phase");
        // Continue with error handling...
    }

    // Example 2: Using macros
    SAFE_EXECUTE("Resource allocation", {
        throw ResourceError(ResourceError::Type::Depleted, "Energy", 100.0, 50.0);
    });

    // Example 3: Chained exception logging
    try {
        try {
            throw std::runtime_error("Database connection lost");
        } catch (const std::exception& e) {
            throw chain_exception("Failed to save mission data", e);
        }

    } catch (const ChainedException& e) {
        ExceptionLogger::logExceptionChain(e, "Data persistence layer");
    }

    ExceptionLogger::disable();
}
```

### 📊 Performance Considerations

#### **Exception Performance Impact**

```cpp
#include <chrono>

class PerformanceTester {
public:
    // Measure exception throwing overhead
    static void measureExceptionOverhead() {
        const int iterations = 100000;

        // Test 1: Normal execution (baseline)
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            volatile int result = i * 2;  // Prevent optimization
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto normal_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        // Test 2: Exception handling (expensive path)
        start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            try {
                throw std::runtime_error("Test exception");
            } catch (const std::exception&) {
                volatile int result = i * 2;  // Same work as baseline
            }
        }

        end = std::chrono::high_resolution_clock::now();
        auto exception_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        // Test 3: Error codes (alternative approach)
        start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            int error_code = -1;  // Simulate error condition
            if (error_code != 0) {
                volatile int result = i * 2;  // Same work
            }
        }

        end = std::chrono::high_resolution_clock::now();
        auto error_code_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "Performance Comparison (" << iterations << " iterations):\n";
        std::cout << "Normal execution: " << normal_time.count() << " μs\n";
        std::cout << "Exception handling: " << exception_time.count() << " μs ("
                  << (exception_time.count() / double(normal_time.count())) << "x slower)\n";
        std::cout << "Error codes: " << error_code_time.count() << " μs ("
                  << (error_code_time.count() / double(normal_time.count())) << "x slower)\n";
    }

    // Compare exception vs error code approaches
    enum class ErrorCode {
        Success = 0,
        InvalidArgument,
        ResourceExhausted,
        OperationFailed
    };

    // Exception-based error handling
    static double divideWithExceptions(double a, double b) {
        if (b == 0.0) {
            throw std::invalid_argument("Division by zero");
        }
        return a / b;
    }

    // Error code-based error handling
    static ErrorCode divideWithErrorCodes(double a, double b, double& result) {
        if (b == 0.0) {
            return ErrorCode::InvalidArgument;
        }
        result = a / b;
        return ErrorCode::Success;
    }

    static void compareErrorHandlingApproaches() {
        const int iterations = 1000000;
        double test_values[][2] = {{10.0, 2.0}, {5.0, 0.0}, {8.0, 4.0}, {7.0, 0.0}};
        const size_t num_tests = sizeof(test_values) / sizeof(test_values[0]);

        // Exception approach
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            auto& test = test_values[i % num_tests];

            try {
                volatile double result = divideWithExceptions(test[0], test[1]);
            } catch (const std::exception&) {
                // Handle error
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto exception_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        // Error code approach
        start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            auto& test = test_values[i % num_tests];
            double result;

            ErrorCode error = divideWithErrorCodes(test[0], test[1], result);
            if (error != ErrorCode::Success) {
                // Handle error
            }
        }

        end = std::chrono::high_resolution_clock::now();
        auto error_code_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "\nError Handling Approach Comparison:\n";
        std::cout << "Exceptions: " << exception_time.count() << " μs\n";
        std::cout << "Error codes: " << error_code_time.count() << " μs\n";
        std::cout << "Ratio: " << (double(exception_time.count()) / error_code_time.count()) << "x\n";
    }
};
```

### 🎯 Best Practices Summary

#### **Exception Handling Guidelines**

```cpp
namespace best_practices {

    // 1. Use exceptions for exceptional conditions, not normal flow control

    // BAD: Using exceptions for control flow
    class BadExample {
    public:
        Planet* findPlanet(const std::string& name) {
            for (auto& planet : planets_) {
                if (planet.getName() == name) {
                    return &planet;
                }
            }
            throw std::runtime_error("Planet not found");  // BAD - not exceptional!
        }
    private:
        std::vector<Planet> planets_;
    };

    // GOOD: Use optional or error codes for expected failures
    class GoodExample {
    public:
        std::optional<Planet*> findPlanet(const std::string& name) {
            for (auto& planet : planets_) {
                if (planet.getName() == name) {
                    return &planet;
                }
            }
            return std::nullopt;  // GOOD - expected case
        }

        Planet& getPlanet(const std::string& name) {
            auto planet = findPlanet(name);
            if (!planet) {
                throw std::invalid_argument("Planet '" + name + "' does not exist");  // OK - programming error
            }
            return **planet;
        }

    private:
        std::vector<Planet> planets_;
    };

    // 2. Follow RAII principles for exception safety
    class RAIIExample {
    private:
        std::unique_ptr<Resource> resource_;
        std::vector<std::unique_ptr<Planet>> planets_;

    public:
        void safeOperation() {
            // All resources managed by RAII
            auto temp_resource = std::make_unique<Resource>();

            // Even if this throws, temp_resource is cleaned up
            planets_.push_back(std::make_unique<Planet>("Safe Planet", 100.0));

            // Commit changes only after success
            resource_ = std::move(temp_resource);
        }
    };

    // 3. Provide strong exception safety guarantees when possible
    class StrongSafetyExample {
    public:
        void updatePlanets(const std::vector<PlanetData>& new_data) {
            // Build new state first
            std::vector<std::unique_ptr<Planet>> new_planets;

            for (const auto& data : new_data) {
                new_planets.push_back(std::make_unique<Planet>(data.name, data.resources));
            }

            // If we reach here, all operations succeeded
            // Atomically replace old state
            planets_ = std::move(new_planets);
        }

    private:
        std::vector<std::unique_ptr<Planet>> planets_;
    };

    // 4. Use appropriate exception types
    void demonstrate_appropriate_exceptions() {
        // Use standard exceptions when they fit
        if (planet_name.empty()) {
            throw std::invalid_argument("Planet name cannot be empty");
        }

        if (resource_amount < 0) {
            throw std::domain_error("Resource amount must be non-negative");
        }

        if (planet_index >= planets.size()) {
            throw std::out_of_range("Planet index out of range");
        }

        // Create custom exceptions for domain-specific errors
        if (insufficient_fuel_for_mission) {
            throw MissionError(MissionError::Type::InsufficientResources, "Not enough fuel");
        }
    }

    // 5. Never throw from destructors
    class DestructorSafety {
    private:
        Resource* resource_;

    public:
        ~DestructorSafety() noexcept {  // Always mark destructors noexcept
            try {
                if (resource_) {
                    cleanup_resource(resource_);
                }
            } catch (...) {
                // Log error but never throw
                // In real code, use proper logging
            }
        }
    };

    // 6. Catch exceptions by const reference
    void proper_catching() {
        try {
            risky_operation();

        } catch (const SpecificException& e) {  // const reference - GOOD
            handle_specific(e);

        } catch (const std::exception& e) {     // const reference - GOOD
            handle_general(e);
        }
        // Never catch by value - causes slicing and extra copying
    }

    // 7. Use function-try-block for constructor/destructor exceptions
    class ConstructorExceptionHandling {
    private:
        std::unique_ptr<Resource> resource1_;
        std::unique_ptr<Resource> resource2_;

    public:
        ConstructorExceptionHandling(int param1, int param2)
        try : resource1_(std::make_unique<Resource>(param1)),
              resource2_(std::make_unique<Resource>(param2)) {

            // Constructor body
            initialize_resources();

        } catch (const std::exception& e) {
            // Handle constructor exceptions
            // Note: object is not constructed if exception occurs
            std::cout << "Construction failed: " << e.what() << "\n";
            throw;  // Must re-throw or throw different exception
        }
    };
}
```

### 📋 Exception Handling Quick Reference

| Scenario                  | Best Practice                  | Example                                                  |
| ------------------------- | ------------------------------ | -------------------------------------------------------- |
| **Input validation**      | `std::invalid_argument`        | `throw std::invalid_argument("Empty name")`              |
| **Range checking**        | `std::out_of_range`            | `throw std::out_of_range("Index too large")`             |
| **Math domain errors**    | `std::domain_error`            | `throw std::domain_error("Negative sqrt")`               |
| **Resource exhaustion**   | `std::runtime_error` or custom | `throw ResourceError::Depleted`                          |
| **System failures**       | `std::system_error`            | `throw std::system_error(errno, std::system_category())` |
| **Logic errors**          | `std::logic_error`             | `throw std::logic_error("Invalid state")`                |
| **Custom domain errors**  | Custom exception class         | `throw MissionError::InvalidTarget`                      |
| **Constructor failures**  | Function-try-block             | `try : member_(init) { } catch { }`                      |
| **Destructor cleanup**    | Never throw, use `noexcept`    | `~Class() noexcept { try { } catch { } }`                |
| **Exception translation** | Catch and rethrow              | `catch(LibError& e) { throw AppError; }`                 |

### 🎯 Key Takeaways

#### **Golden Rules**

1. **Exceptions are for exceptional conditions** - Not for normal program flow
2. **RAII ensures exception safety** - Use smart pointers and automatic cleanup
3. **Provide strong guarantees when possible** - All-or-nothing operations
4. **Never throw from destructors** - Mark them `noexcept`
5. **Catch by const reference** - Avoid slicing and unnecessary copies
6. **Use appropriate exception types** - Match the error to the exception
7. **Document exception specifications** - Use `noexcept` when appropriate
8. **Consider performance implications** - Exceptions have overhead when thrown
9. **Test exception paths** - Ensure error handling works correctly
10. **Log exceptions appropriately** - Aid in debugging and monitoring

#### **When to Use Exceptions vs Alternatives**

**Use Exceptions When:**

- Error is unexpected/exceptional
- Error handling needs to cross multiple call levels
- You want strong type safety for errors
- Performance isn't critical on error paths
- You need rich error information

**Consider Alternatives When:**

- Errors are expected/common
- Performance is critical
- You need C compatibility
- Error handling is local
- Simple binary success/failure is sufficient

---

_This cheat sheet provides comprehensive coverage of C++ exception handling. For complete implementations and examples, see the `src/` directory throughout CppVerseHub._
