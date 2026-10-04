/**
 * @file RAII_Examples.hpp
 * @brief Resource Acquisition Is Initialisation: ownership wrappers and scope guards.
 *
 * Demonstrates how C++ ties resource lifetime to object lifetime:
 *  - UniqueHandle<Traits>: a generic move-only owner for C-style handles (file descriptors,
 *    OS handles), illustrated with an in-process mock handle table.
 *  - FileRAII: rule-of-zero file wrapper built on `std::unique_ptr<FILE, Closer>`.
 *  - TimerRAII: scoped timing that reports through a callback on destruction.
 *  - ScopedLock<Mutex>: a lock_guard/unique_lock style lock owner.
 *  - ScopeGuard / ScopeFail / ScopeSuccess: run cleanup on scope exit, on exception only,
 *    or on normal exit only (`std::uncaught_exceptions`).
 *  - ResourcePool<R>: leases pooled resources that automatically return on destruction.
 *  - NetworkConnection: a mock connection showing acquisition-in-constructor and that a
 *    throwing constructor leaks nothing.
 *  - RAIIUtils::ArrayRAII<T>: a hand-written rule-of-five container with the strong
 *    exception guarantee (copy-and-swap) built on uninitialised-memory algorithms.
 *
 * Why: RAII makes cleanup deterministic and exception safe without try/finally.
 */

#ifndef CPPVERSEHUB_MEMORY_RAII_EXAMPLES_HPP
#define CPPVERSEHUB_MEMORY_RAII_EXAMPLES_HPP

#include <atomic>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace CppVerseHub::Memory {

/**
 * @brief Requirements for UniqueHandle traits.
 */
template <typename Traits>
concept HandleTraits = requires(typename Traits::handle_type h) {
    typename Traits::handle_type;
    { Traits::invalid() } noexcept -> std::same_as<typename Traits::handle_type>;
    { Traits::close(h) } noexcept;
};

/**
 * @class UniqueHandle
 * @brief Generic move-only owner of a C-style handle.
 * @tparam Traits Provides handle_type, invalid() and close().
 */
template <HandleTraits Traits>
class UniqueHandle {
public:
    using handle_type = typename Traits::handle_type;

    /** @brief Construct an empty (invalid) handle. */
    UniqueHandle() noexcept = default;

    /**
     * @brief Adopt ownership of @p h.
     * @param h Handle to own.
     */
    explicit UniqueHandle(handle_type h) noexcept : handle_(h) {}

    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;

    /**
     * @brief Transfer ownership from @p other.
     * @param other Source, left invalid.
     */
    UniqueHandle(UniqueHandle&& other) noexcept : handle_(other.release()) {}

    /**
     * @brief Close the current handle and take ownership of @p other's.
     * @param other Source, left invalid.
     * @return *this.
     */
    UniqueHandle& operator=(UniqueHandle&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    /** @brief Close the owned handle, if any. */
    ~UniqueHandle() { reset(); }

    /** @brief @return The raw handle (ownership retained). */
    [[nodiscard]] handle_type get() const noexcept { return handle_; }
    /** @brief @return True if a valid handle is owned. */
    [[nodiscard]] bool valid() const noexcept { return handle_ != Traits::invalid(); }
    /** @brief @return valid(). */
    explicit operator bool() const noexcept { return valid(); }

    /**
     * @brief Give up ownership without closing.
     * @return The previously owned handle.
     */
    [[nodiscard]] handle_type release() noexcept { return std::exchange(handle_, Traits::invalid()); }

    /**
     * @brief Close the current handle and own @p h instead.
     * @param h New handle (defaults to invalid).
     */
    void reset(handle_type h = Traits::invalid()) noexcept {
        const handle_type old = std::exchange(handle_, h);
        if (old != Traits::invalid()) {
            Traits::close(old);
        }
    }

private:
    handle_type handle_ = Traits::invalid();
};

/**
 * @brief Mock "operating system" handle table used to demonstrate UniqueHandle portably.
 */
struct MockOsHandleTraits {
    using handle_type = int;

    /** @brief @return Sentinel for "no handle". */
    static constexpr int invalid() noexcept { return -1; }

    /**
     * @brief Open a new mock handle.
     * @return Fresh handle id (>= 0).
     */
    [[nodiscard]] static int open() noexcept {
        open_count_.fetch_add(1, std::memory_order_relaxed);
        return next_id_.fetch_add(1, std::memory_order_relaxed);
    }

    /**
     * @brief Close a mock handle.
     * @param h Handle to close.
     */
    static void close(int h) noexcept {
        if (h >= 0) {
            open_count_.fetch_sub(1, std::memory_order_relaxed);
        }
    }

    /** @brief @return Number of handles currently open. */
    [[nodiscard]] static int open_count() noexcept { return open_count_.load(); }

private:
    inline static std::atomic<int> next_id_{0};
    inline static std::atomic<int> open_count_{0};
};

/** @brief UniqueHandle over the mock handle table. */
using MockOsHandle = UniqueHandle<MockOsHandleTraits>;

/**
 * @class FileRAII
 * @brief Rule-of-zero file wrapper: `std::unique_ptr<FILE, Closer>` does all the work.
 */
class FileRAII {
public:
    /**
     * @brief Open @p path with a C `fopen` mode string.
     * @param path File to open.
     * @param mode Mode such as "w", "r", "a+", "wb".
     * @throws std::runtime_error if the file cannot be opened.
     */
    FileRAII(const std::filesystem::path& path, const char* mode);

    /**
     * @brief Write @p data.
     * @param data Bytes to write.
     * @return True if all bytes were written.
     */
    bool write(std::string_view data);

    /**
     * @brief Read the whole file from the beginning.
     * @return File contents.
     * @throws std::runtime_error if the file is closed or a read fails.
     */
    [[nodiscard]] std::string read_all();

    /** @brief Flush buffered output. @return True on success. */
    bool flush() noexcept;

    /** @brief Close early. @return True if the close succeeded (or already closed). */
    bool close() noexcept;

    /** @brief @return True while the file is open. */
    [[nodiscard]] bool is_open() const noexcept { return file_ != nullptr; }
    /** @brief @return Path the file was opened with. */
    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

private:
    struct Closer {
        void operator()(std::FILE* f) const noexcept { static_cast<void>(std::fclose(f)); }
    };

    std::unique_ptr<std::FILE, Closer> file_;
    std::filesystem::path path_;
};

/**
 * @class TimerRAII
 * @brief Measures the lifetime of a scope and reports it through a callback.
 */
class TimerRAII {
public:
    using Clock = std::chrono::steady_clock;
    using Callback = std::function<void(std::chrono::nanoseconds)>;

    /**
     * @brief Start timing.
     * @param on_stop Invoked with the elapsed time on destruction (may be empty).
     */
    explicit TimerRAII(Callback on_stop = {}) : on_stop_(std::move(on_stop)), start_(Clock::now()) {}

    TimerRAII(const TimerRAII&) = delete;
    TimerRAII& operator=(const TimerRAII&) = delete;
    TimerRAII(TimerRAII&&) = delete;
    TimerRAII& operator=(TimerRAII&&) = delete;

    /** @brief Stop timing and report; callback exceptions are swallowed. */
    ~TimerRAII() {
        if (on_stop_) {
            try {
                on_stop_(elapsed());
            } catch (...) { // destructors must not throw
            }
        }
    }

    /** @brief @return Time since construction. */
    [[nodiscard]] std::chrono::nanoseconds elapsed() const noexcept {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start_);
    }

private:
    Callback on_stop_;
    Clock::time_point start_;
};

/**
 * @brief Types with lock() / unlock().
 */
template <typename M>
concept BasicLockable = requires(M m) {
    m.lock();
    m.unlock();
};

/**
 * @class ScopedLock
 * @brief Owns a lock on a mutex for the lifetime of the object (with early unlock).
 * @tparam Mutex Any BasicLockable type.
 */
template <BasicLockable Mutex>
class ScopedLock {
public:
    /**
     * @brief Lock @p mutex.
     * @param mutex Mutex to lock; must outlive this object.
     */
    explicit ScopedLock(Mutex& mutex) : mutex_(mutex) { lock(); }

    ScopedLock(const ScopedLock&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;
    ScopedLock(ScopedLock&&) = delete;
    ScopedLock& operator=(ScopedLock&&) = delete;

    /** @brief Unlock if still owned. */
    ~ScopedLock() {
        if (owns_) {
            mutex_.unlock();
        }
    }

    /** @brief Re-acquire after unlock() (no-op if already owned). */
    void lock() {
        if (!owns_) {
            mutex_.lock();
            owns_ = true;
        }
    }

    /** @brief Release early (no-op if not owned). */
    void unlock() {
        if (owns_) {
            mutex_.unlock();
            owns_ = false;
        }
    }

    /** @brief @return True while the lock is held. */
    [[nodiscard]] bool owns_lock() const noexcept { return owns_; }

private:
    Mutex& mutex_;
    bool owns_ = false;
};

/**
 * @brief When a scope guard fires.
 */
enum class GuardPolicy {
    Always,    ///< On every scope exit.
    OnFailure, ///< Only when leaving the scope because of an exception.
    OnSuccess  ///< Only when leaving the scope normally.
};

/**
 * @class BasicScopeGuard
 * @brief Runs a callable when the scope is left, subject to a policy.
 *
 * The callable must not throw (destructors are noexcept). Moved-from or dismissed
 * guards do nothing.
 *
 * @tparam F Nullary callable.
 * @tparam Policy When to fire.
 */
template <typename F, GuardPolicy Policy>
class BasicScopeGuard {
    static_assert(std::is_nothrow_move_constructible_v<F>, "guard callable must be nothrow movable");

public:
    /**
     * @brief Arm the guard.
     * @param f Cleanup action.
     */
    explicit BasicScopeGuard(F f) noexcept
        : cleanup_(std::move(f)), exceptions_on_entry_(std::uncaught_exceptions()) {}

    /**
     * @brief Move the responsibility from @p other.
     * @param other Source guard, dismissed afterwards.
     */
    BasicScopeGuard(BasicScopeGuard&& other) noexcept
        : cleanup_(std::move(other.cleanup_))
        , exceptions_on_entry_(other.exceptions_on_entry_)
        , active_(std::exchange(other.active_, false)) {}

    BasicScopeGuard(const BasicScopeGuard&) = delete;
    BasicScopeGuard& operator=(const BasicScopeGuard&) = delete;
    BasicScopeGuard& operator=(BasicScopeGuard&&) = delete;

    /** @brief Fire according to the policy. */
    ~BasicScopeGuard() {
        if (!active_) {
            return;
        }
        const bool unwinding = std::uncaught_exceptions() > exceptions_on_entry_;
        if constexpr (Policy == GuardPolicy::Always) {
            cleanup_();
        } else if constexpr (Policy == GuardPolicy::OnFailure) {
            if (unwinding) {
                cleanup_();
            }
        } else {
            if (!unwinding) {
                cleanup_();
            }
        }
    }

    /** @brief Disarm (e.g. after a transaction commits). */
    void dismiss() noexcept { active_ = false; }
    /** @brief @return True if the guard is still armed. */
    [[nodiscard]] bool active() const noexcept { return active_; }

private:
    F cleanup_;
    int exceptions_on_entry_;
    bool active_ = true;
};

/** @brief Guard that always runs. */
template <typename F>
using ScopeGuard = BasicScopeGuard<F, GuardPolicy::Always>;
/** @brief Guard that runs only during exception unwinding. */
template <typename F>
using ScopeFail = BasicScopeGuard<F, GuardPolicy::OnFailure>;
/** @brief Guard that runs only on normal scope exit. */
template <typename F>
using ScopeSuccess = BasicScopeGuard<F, GuardPolicy::OnSuccess>;

/**
 * @brief Create a ScopeGuard.
 * @param f Cleanup action.
 * @return Armed guard.
 */
template <typename F>
[[nodiscard]] ScopeGuard<std::decay_t<F>> make_scope_guard(F&& f) {
    return ScopeGuard<std::decay_t<F>>(std::forward<F>(f));
}

/**
 * @brief Create a ScopeFail guard.
 * @param f Rollback action.
 * @return Armed guard.
 */
template <typename F>
[[nodiscard]] ScopeFail<std::decay_t<F>> make_scope_fail(F&& f) {
    return ScopeFail<std::decay_t<F>>(std::forward<F>(f));
}

/**
 * @brief Create a ScopeSuccess guard.
 * @param f Commit action.
 * @return Armed guard.
 */
template <typename F>
[[nodiscard]] ScopeSuccess<std::decay_t<F>> make_scope_success(F&& f) {
    return ScopeSuccess<std::decay_t<F>>(std::forward<F>(f));
}

/**
 * @class ResourcePool
 * @brief Thread-safe pool of reusable resources leased through RAII handles.
 * @tparam Resource Pooled type.
 */
template <typename Resource>
class ResourcePool {
public:
    using Factory = std::function<std::unique_ptr<Resource>()>;

    /**
     * @class Lease
     * @brief Move-only handle; returns the resource to the pool on destruction.
     */
    class Lease {
    public:
        Lease(const Lease&) = delete;
        Lease& operator=(const Lease&) = delete;

        /**
         * @brief Transfer the lease.
         * @param other Source, left empty.
         */
        Lease(Lease&& other) noexcept
            : pool_(std::exchange(other.pool_, nullptr)), resource_(std::move(other.resource_)) {}

        /**
         * @brief Return the current resource and take over @p other's.
         * @param other Source, left empty.
         * @return *this.
         */
        Lease& operator=(Lease&& other) noexcept {
            if (this != &other) {
                give_back();
                pool_ = std::exchange(other.pool_, nullptr);
                resource_ = std::move(other.resource_);
            }
            return *this;
        }

        /** @brief Return the resource to the pool. */
        ~Lease() { give_back(); }

        /** @brief @return The leased resource. */
        [[nodiscard]] Resource& operator*() const noexcept { return *resource_; }
        /** @brief @return Pointer to the leased resource. */
        [[nodiscard]] Resource* operator->() const noexcept { return resource_.get(); }
        /** @brief @return True if this lease holds a resource. */
        explicit operator bool() const noexcept { return resource_ != nullptr; }

    private:
        friend class ResourcePool;
        Lease(ResourcePool* pool, std::unique_ptr<Resource> r) noexcept
            : pool_(pool), resource_(std::move(r)) {}

        void give_back() noexcept {
            if (pool_ != nullptr && resource_) {
                pool_->give_back(std::move(resource_));
            }
            pool_ = nullptr;
        }

        ResourcePool* pool_;
        std::unique_ptr<Resource> resource_;
    };

    /**
     * @brief Create a pool with @p initial_size pre-built resources.
     * @param initial_size Resources created eagerly.
     * @param factory Creates new resources (default: std::make_unique<Resource>()).
     */
    explicit ResourcePool(std::size_t initial_size = 0, Factory factory = {})
        : factory_(factory ? std::move(factory) : Factory([] { return std::make_unique<Resource>(); })) {
        idle_.reserve(initial_size);
        for (std::size_t i = 0; i < initial_size; ++i) {
            idle_.push_back(factory_());
        }
        created_ = initial_size;
    }

    ResourcePool(const ResourcePool&) = delete;
    ResourcePool& operator=(const ResourcePool&) = delete;
    ResourcePool(ResourcePool&&) = delete;
    ResourcePool& operator=(ResourcePool&&) = delete;
    ~ResourcePool() = default;

    /**
     * @brief Lease an idle resource, creating one if none is idle.
     * @return Lease that returns the resource on destruction (must not outlive the pool).
     */
    [[nodiscard]] Lease acquire() {
        std::unique_lock lock(mutex_);
        if (!idle_.empty()) {
            std::unique_ptr<Resource> r = std::move(idle_.back());
            idle_.pop_back();
            ++leased_;
            return Lease(this, std::move(r));
        }
        lock.unlock(); // do not hold the lock while running user code
        std::unique_ptr<Resource> r = factory_();
        lock.lock();
        ++created_;
        ++leased_;
        return Lease(this, std::move(r));
    }

    /** @brief @return Idle resources. */
    [[nodiscard]] std::size_t idle_count() const {
        const std::lock_guard lock(mutex_);
        return idle_.size();
    }
    /** @brief @return Resources currently leased. */
    [[nodiscard]] std::size_t leased_count() const {
        const std::lock_guard lock(mutex_);
        return leased_;
    }
    /** @brief @return Resources ever created. */
    [[nodiscard]] std::size_t created_count() const {
        const std::lock_guard lock(mutex_);
        return created_;
    }

private:
    void give_back(std::unique_ptr<Resource> r) noexcept {
        const std::lock_guard lock(mutex_);
        --leased_;
        try {
            idle_.push_back(std::move(r));
        } catch (...) { // out of memory: the resource is destroyed instead of pooled
        }
    }

    mutable std::mutex mutex_;
    Factory factory_;
    std::vector<std::unique_ptr<Resource>> idle_;
    std::size_t leased_ = 0;
    std::size_t created_ = 0;
};

/**
 * @class NetworkConnection
 * @brief Mock connection: "connects" in the constructor and "disconnects" in the destructor.
 */
class NetworkConnection {
public:
    /**
     * @brief Connect to @p address.
     * @param address "host:port"; must contain a ':'.
     * @throws std::invalid_argument if the address is malformed (nothing is acquired).
     */
    explicit NetworkConnection(std::string address);

    NetworkConnection(const NetworkConnection&) = delete;
    NetworkConnection& operator=(const NetworkConnection&) = delete;

    /**
     * @brief Take over @p other's connection.
     * @param other Source, left disconnected.
     */
    NetworkConnection(NetworkConnection&& other) noexcept;

    /**
     * @brief Disconnect, then take over @p other's connection.
     * @param other Source, left disconnected.
     * @return *this.
     */
    NetworkConnection& operator=(NetworkConnection&& other) noexcept;

    /** @brief Disconnect if connected. */
    ~NetworkConnection();

    /**
     * @brief Send a message.
     * @param message Payload.
     * @return Bytes "sent".
     * @throws std::logic_error if not connected.
     */
    std::size_t send(std::string_view message);

    /** @brief Disconnect early. */
    void disconnect() noexcept;

    /** @brief @return True while connected. */
    [[nodiscard]] bool connected() const noexcept { return connected_; }
    /** @brief @return Remote address. */
    [[nodiscard]] const std::string& address() const noexcept { return address_; }
    /** @brief @return Bytes sent over this connection. */
    [[nodiscard]] std::size_t bytes_sent() const noexcept { return bytes_sent_; }
    /** @brief @return Connections currently open process-wide. */
    [[nodiscard]] static int active_connections() noexcept { return active_.load(); }

private:
    std::string address_;
    std::size_t bytes_sent_ = 0;
    bool connected_ = false;
    inline static std::atomic<int> active_{0};
};

/**
 * @brief Utilities built on RAII.
 */
namespace RAIIUtils {

/**
 * @brief Run @p func while timing it.
 * @param elapsed Receives the duration.
 * @param func Callable to run.
 * @return Whatever @p func returns.
 */
template <typename F>
decltype(auto) measure(std::chrono::nanoseconds& elapsed, F&& func) {
    const TimerRAII timer([&elapsed](std::chrono::nanoseconds d) { elapsed = d; });
    return std::forward<F>(func)();
}

/**
 * @class ArrayRAII
 * @brief Fixed-size heap array with full rule-of-five and strong exception guarantee.
 * @tparam T Element type.
 */
template <typename T>
class ArrayRAII {
public:
    using value_type = T;
    using size_type = std::size_t;
    using iterator = T*;
    using const_iterator = const T*;

    /** @brief Construct an empty array. */
    ArrayRAII() noexcept = default;

    /**
     * @brief Construct @p size copies of @p value.
     * @param size Element count.
     * @param value Prototype element.
     * @throws Anything T's copy constructor or the allocator throws (no leak).
     */
    explicit ArrayRAII(size_type size, const T& value = T()) : data_(allocate(size)), size_(size) {
        try {
            std::uninitialized_fill_n(data_, size_, value);
        } catch (...) {
            deallocate(data_, size_);
            throw;
        }
    }

    /**
     * @brief Deep copy.
     * @param other Source.
     * @throws Anything T's copy constructor throws (no leak, source untouched).
     */
    ArrayRAII(const ArrayRAII& other) : data_(allocate(other.size_)), size_(other.size_) {
        try {
            std::uninitialized_copy_n(other.data_, other.size_, data_);
        } catch (...) {
            deallocate(data_, size_);
            throw;
        }
    }

    /**
     * @brief Steal @p other's buffer.
     * @param other Source, left empty.
     */
    ArrayRAII(ArrayRAII&& other) noexcept
        : data_(std::exchange(other.data_, nullptr)), size_(std::exchange(other.size_, 0)) {}

    /**
     * @brief Copy-and-swap assignment (strong guarantee).
     * @param other Source.
     * @return *this.
     */
    ArrayRAII& operator=(const ArrayRAII& other) {
        if (this != &other) {
            ArrayRAII copy(other); // may throw; *this unchanged
            swap(copy);
        }
        return *this;
    }

    /**
     * @brief Move assignment.
     * @param other Source, left empty.
     * @return *this.
     */
    ArrayRAII& operator=(ArrayRAII&& other) noexcept {
        if (this != &other) {
            ArrayRAII tmp(std::move(other));
            swap(tmp);
        }
        return *this;
    }

    /** @brief Destroy elements and free the buffer. */
    ~ArrayRAII() {
        std::destroy_n(data_, size_);
        deallocate(data_, size_);
    }

    /**
     * @brief Swap contents with @p other.
     * @param other Array to swap with.
     */
    void swap(ArrayRAII& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
    }

    /**
     * @brief Unchecked element access.
     * @param i Index (< size()).
     * @return Element reference.
     */
    [[nodiscard]] T& operator[](size_type i) noexcept { return data_[i]; }
    /**
     * @brief Unchecked element access.
     * @param i Index (< size()).
     * @return Element reference.
     */
    [[nodiscard]] const T& operator[](size_type i) const noexcept { return data_[i]; }

    /**
     * @brief Checked element access.
     * @param i Index.
     * @return Element reference.
     * @throws std::out_of_range if @p i >= size().
     */
    [[nodiscard]] T& at(size_type i) {
        if (i >= size_) {
            throw std::out_of_range("ArrayRAII::at: index out of range");
        }
        return data_[i];
    }

    /** @brief @return Element count. */
    [[nodiscard]] size_type size() const noexcept { return size_; }
    /** @brief @return True if empty. */
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    /** @brief @return Pointer to the first element. */
    [[nodiscard]] T* data() noexcept { return data_; }
    /** @brief @return Pointer to the first element. */
    [[nodiscard]] const T* data() const noexcept { return data_; }
    /** @brief @return Begin iterator. */
    [[nodiscard]] iterator begin() noexcept { return data_; }
    /** @brief @return End iterator. */
    [[nodiscard]] iterator end() noexcept { return data_ + size_; }
    /** @brief @return Begin iterator. */
    [[nodiscard]] const_iterator begin() const noexcept { return data_; }
    /** @brief @return End iterator. */
    [[nodiscard]] const_iterator end() const noexcept { return data_ + size_; }

private:
    [[nodiscard]] static T* allocate(size_type n) {
        return n == 0 ? nullptr : std::allocator<T>{}.allocate(n);
    }
    static void deallocate(T* p, size_type n) noexcept {
        if (p != nullptr) {
            std::allocator<T>{}.deallocate(p, n);
        }
    }

    T* data_ = nullptr;
    size_type size_ = 0;
};

} // namespace RAIIUtils

/**
 * @brief Showcase: every RAII wrapper in this header, including exception paths.
 * @param out Stream receiving the narration.
 */
void demonstrateRAII(std::ostream& out = std::cout);

} // namespace CppVerseHub::Memory

#endif // CPPVERSEHUB_MEMORY_RAII_EXAMPLES_HPP
