/**
 * @file RAII_Examples.hpp
 * @brief Comprehensive RAII (Resource Acquisition Is Initialization) examples
 * @details File location: src/memory/RAII_Examples.hpp
 * 
 * This file demonstrates various RAII patterns including file handling,
 * memory management, locks, network connections, and custom RAII wrappers.
 */

#ifndef RAII_EXAMPLES_HPP
#define RAII_EXAMPLES_HPP

#include <memory>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <thread>
#include <functional>
#include <exception>
#include <cstdio>
#include <cassert>

namespace CppVerseHub::Memory {

    /**
     * @class FileRAII
     * @brief RAII wrapper for C-style FILE* handles
     */
    class FileRAII {
    public:
        explicit FileRAII(const std::string& filename, const std::string& mode)
            : file_(std::fopen(filename.c_str(), mode.c_str())), filename_(filename) {
            if (!file_) {
                throw std::runtime_error("Failed to open file: " + filename);
            }
            std::cout << "FileRAII: Opened file '" << filename_ << "'\n";
        }

        ~FileRAII() {
            if (file_) {
                std::fclose(file_);
                std::cout << "FileRAII: Closed file '" << filename_ << "'\n";
            }
        }

        // Non-copyable
        FileRAII(const FileRAII&) = delete;
        FileRAII& operator=(const FileRAII&) = delete;

        // Movable
        FileRAII(FileRAII&& other) noexcept
            : file_(other.file_), filename_(std::move(other.filename_)) {
            other.file_ = nullptr;
        }

        FileRAII& operator=(FileRAII&& other) noexcept {
            if (this != &other) {
                if (file_) {
                    std::fclose(file_);
                }
                file_ = other.file_;
                filename_ = std::move(other.filename_);
                other.file_ = nullptr;
            }
            return *this;
        }

        // Access the underlying FILE*
        FILE* get() const { return file_; }
        
        // Convenience methods
        bool write(const std::string& data) {
            return file_ && std::fwrite(data.c_str(), 1, data.size(), file_) == data.size();
        }

        std::string read_all() {
            if (!file_) return "";
            
            std::fseek(file_, 0, SEEK_END);
            long size = std::ftell(file_);
            std::fseek(file_, 0, SEEK_SET);
            
            std::string content(size, '\0');
            std::fread(&content[0], 1, size, file_);
            return content;
        }

        void flush() {
            if (file_) std::fflush(file_);
        }

    private:
        FILE* file_;
        std::string filename_;
    };

    /**
     * @class TimerRAII
     * @brief RAII timer for measuring execution time
     */
    class TimerRAII {
    public:
        explicit TimerRAII(const std::string& operation_name)
            : operation_name_(operation_name), start_time_(std::chrono::high_resolution_clock::now()) {
            std::cout << "TimerRAII: Starting timer for '" << operation_name_ << "'\n";
        }

        ~TimerRAII() {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
            std::cout << "TimerRAII: '" << operation_name_ << "' completed in " 
                      << duration.count() << " microseconds\n";
        }

        // Get elapsed time without destroying the timer
        std::chrono::microseconds elapsed() const {
            auto current_time = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::microseconds>(current_time - start_time_);
        }

    private:
        std::string operation_name_;
        std::chrono::high_resolution_clock::time_point start_time_;
    };

    /**
     * @class ScopedLock
     * @brief Custom RAII lock implementation
     */
    template<typename Mutex>
    class ScopedLock {
    public:
        explicit ScopedLock(Mutex& mutex) : mutex_(mutex), locked_(false) {
            lock();
        }

        ~ScopedLock() {
            if (locked_) {
                unlock();
            }
        }

        // Non-copyable, non-movable for safety
        ScopedLock(const ScopedLock&) = delete;
        ScopedLock& operator=(const ScopedLock&) = delete;
        ScopedLock(ScopedLock&&) = delete;
        ScopedLock& operator=(ScopedLock&&) = delete;

        void lock() {
            if (!locked_) {
                mutex_.lock();
                locked_ = true;
                std::cout << "ScopedLock: Mutex locked\n";
            }
        }

        void unlock() {
            if (locked_) {
                mutex_.unlock();
                locked_ = false;
                std::cout << "ScopedLock: Mutex unlocked\n";
            }
        }

        bool is_locked() const { return locked_; }

    private:
        Mutex& mutex_;
        bool locked_;
    };

    /**
     * @class ResourcePool
     * @brief RAII-managed resource pool
     */
    template<typename Resource>
    class ResourcePool {
    private:
        struct PooledResource {
            std::unique_ptr<Resource> resource;
            bool in_use = false;
        };

    public:
        explicit ResourcePool(size_t initial_size = 10) {
            pool_.reserve(initial_size);
            for (size_t i = 0; i < initial_size; ++i) {
                pool_.push_back({std::make_unique<Resource>(), false});
            }
            std::cout << "ResourcePool: Created with " << initial_size << " resources\n";
        }

        ~ResourcePool() {
            std::cout << "ResourcePool: Destroying pool with " << pool_.size() << " resources\n";
        }

        class ResourceHandle {
        public:
            ResourceHandle(ResourcePool& pool, size_t index)
                : pool_(pool), index_(index) {}

            ~ResourceHandle() {
                if (index_ != SIZE_MAX) {
                    pool_.return_resource(index_);
                }
            }

            // Non-copyable but movable
            ResourceHandle(const ResourceHandle&) = delete;
            ResourceHandle& operator=(const ResourceHandle&) = delete;

            ResourceHandle(ResourceHandle&& other) noexcept
                : pool_(other.pool_), index_(other.index_) {
                other.index_ = SIZE_MAX;
            }

            ResourceHandle& operator=(ResourceHandle&& other) noexcept {
                if (this != &other) {
                    if (index_ != SIZE_MAX) {
                        pool_.return_resource(index_);
                    }
                    index_ = other.index_;
                    other.index_ = SIZE_MAX;
                }
                return *this;
            }

            Resource* operator->() const {
                return index_ != SIZE_MAX ? pool_.pool_[index_].resource.get() : nullptr;
            }

            Resource& operator*() const {
                assert(index_ != SIZE_MAX);
                return *pool_.pool_[index_].resource;
            }

            bool is_valid() const { return index_ != SIZE_MAX; }

        private:
            ResourcePool& pool_;
            size_t index_;
        };

        ResourceHandle acquire() {
            std::lock_guard<std::mutex> lock(mutex_);
            
            for (size_t i = 0; i < pool_.size(); ++i) {
                if (!pool_[i].in_use) {
                    pool_[i].in_use = true;
                    std::cout << "ResourcePool: Acquired resource " << i << "\n";
                    return ResourceHandle(*this, i);
                }
            }

            // No available resource, create a new one
            pool_.push_back({std::make_unique<Resource>(), true});
            size_t index = pool_.size() - 1;
            std::cout << "ResourcePool: Created and acquired new resource " << index << "\n";
            return ResourceHandle(*this, index);
        }

        size_t size() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return pool_.size();
        }

        size_t available_count() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return std::count_if(pool_.begin(), pool_.end(), 
                                [](const PooledResource& pr) { return !pr.in_use; });
        }

    private:
        void return_resource(size_t index) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (index < pool_.size()) {
                pool_[index].in_use = false;
                std::cout << "ResourcePool: Returned resource " << index << "\n";
            }
        }

        std::vector<PooledResource> pool_;
        mutable std::mutex mutex_;
        
        friend class ResourceHandle;
    };

    /**
     * @class NetworkConnection
     * @brief Mock network connection for RAII demonstration
     */
    class NetworkConnection {
    public:
        explicit NetworkConnection(const std::string& address, int port)
            : address_(address), port_(port), connected_(false) {
            connect();
        }

        ~NetworkConnection() {
            disconnect();
        }

        // Non-copyable but movable
        NetworkConnection(const NetworkConnection&) = delete;
        NetworkConnection& operator=(const NetworkConnection&) = delete;

        NetworkConnection(NetworkConnection&& other) noexcept
            : address_(std::move(other.address_)), port_(other.port_), connected_(other.connected_) {
            other.connected_ = false;
        }

        NetworkConnection& operator=(NetworkConnection&& other) noexcept {
            if (this != &other) {
                disconnect();
                address_ = std::move(other.address_);
                port_ = other.port_;
                connected_ = other.connected_;
                other.connected_ = false;
            }
            return *this;
        }

        bool send_data(const std::string& data) {
            if (!connected_) return false;
            
            std::cout << "NetworkConnection: Sending " << data.size() 
                      << " bytes to " << address_ << ":" << port_ << "\n";
            return true;
        }

        std::string receive_data() {
            if (!connected_) return "";
            
            std::cout << "NetworkConnection: Receiving data from " 
                      << address_ << ":" << port_ << "\n";
            return "Mock received data";
        }

        bool is_connected() const { return connected_; }

    private:
        void connect() {
            std::cout << "NetworkConnection: Connecting to " 
                      << address_ << ":" << port_ << "\n";
            // Simulate connection time
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            connected_ = true;
            std::cout << "NetworkConnection: Connected successfully\n";
        }

        void disconnect() {
            if (connected_) {
                std::cout << "NetworkConnection: Disconnecting from " 
                          << address_ << ":" << port_ << "\n";
                connected_ = false;
            }
        }

        std::string address_;
        int port_;
        bool connected_;
    };

    /**
     * @class ScopeGuard
     * @brief Generic RAII scope guard for arbitrary cleanup actions
     */
    template<typename F>
    class ScopeGuard {
    public:
        explicit ScopeGuard(F&& cleanup_function)
            : cleanup_(std::forward<F>(cleanup_function)), active_(true) {}

        ~ScopeGuard() {
            if (active_) {
                try {
                    cleanup_();
                } catch (...) {
                    // Destructors should not throw
                    std::cout << "ScopeGuard: Exception in cleanup function\n";
                }
            }
        }

        // Non-copyable but movable
        ScopeGuard(const ScopeGuard&) = delete;
        ScopeGuard& operator=(const ScopeGuard&) = delete;

        ScopeGuard(ScopeGuard&& other) noexcept
            : cleanup_(std::move(other.cleanup_)), active_(other.active_) {
            other.active_ = false;
        }

        ScopeGuard& operator=(ScopeGuard&& other) noexcept {
            if (this != &other) {
                if (active_) {
                    cleanup_();
                }
                cleanup_ = std::move(other.cleanup_);
                active_ = other.active_;
                other.active_ = false;
            }
            return *this;
        }

        void dismiss() {
            active_ = false;
        }

        bool is_active() const { return active_; }

    private:
        F cleanup_;
        bool active_;
    };

    /**
     * @brief Helper function to create scope guards
     */
    template<typename F>
    ScopeGuard<F> make_scope_guard(F&& cleanup_function) {
        return ScopeGuard<F>(std::forward<F>(cleanup_function));
    }

    /**
     * @class MemoryMapper
     * @brief RAII wrapper for memory-mapped files (mock implementation)
     */
    class MemoryMapper {
    public:
        explicit MemoryMapper(const std::string& filename, size_t size)
            : filename_(filename), size_(size), mapped_memory_(nullptr) {
            // Mock memory mapping
            mapped_memory_ = new char[size_];
            std::cout << "MemoryMapper: Mapped " << size_ 
                      << " bytes from file '" << filename_ << "'\n";
        }

        ~MemoryMapper() {
            if (mapped_memory_) {
                delete[] mapped_memory_;
                std::cout << "MemoryMapper: Unmapped memory for file '" 
                          << filename_ << "'\n";
            }
        }

        // Non-copyable but movable
        MemoryMapper(const MemoryMapper&) = delete;
        MemoryMapper& operator=(const MemoryMapper&) = delete;

        MemoryMapper(MemoryMapper&& other) noexcept
            : filename_(std::move(other.filename_)), size_(other.size_), 
              mapped_memory_(other.mapped_memory_) {
            other.mapped_memory_ = nullptr;
            other.size_ = 0;
        }

        MemoryMapper& operator=(MemoryMapper&& other) noexcept {
            if (this != &other) {
                if (mapped_memory_) {
                    delete[] mapped_memory_;
                }
                filename_ = std::move(other.filename_);
                size_ = other.size_;
                mapped_memory_ = other.mapped_memory_;
                other.mapped_memory_ = nullptr;
                other.size_ = 0;
            }
            return *this;
        }

        char* data() const { return mapped_memory_; }
        size_t size() const { return size_; }

        void sync() {
            if (mapped_memory_) {
                std::cout << "MemoryMapper: Synchronizing mapped memory to disk\n";
            }
        }

    private:
        std::string filename_;
        size_t size_;
        char* mapped_memory_;
    };

    /**
     * @class RAIIDemo
     * @brief Comprehensive demonstration of RAII patterns
     */
    class RAIIDemo {
    public:
        RAIIDemo() = default;

        void demonstrateFileRAII();
        void demonstrateTimerRAII();
        void demonstrateScopedLock();
        void demonstrateResourcePool();
        void demonstrateNetworkConnection();
        void demonstrateScopeGuard();
        void demonstrateMemoryMapper();
        void demonstrateExceptionSafety();
        void demonstrateNestedRAII();
        void demonstrateRAIIWithSmartPointers();

        void runAllDemonstrations();

    private:
        // Test resource for pool demonstration
        class TestResource {
        public:
            TestResource() : id_(++counter_) {
                std::cout << "TestResource " << id_ << " created\n";
            }
            
            ~TestResource() {
                std::cout << "TestResource " << id_ << " destroyed\n";
            }
            
            void do_work() {
                std::cout << "TestResource " << id_ << " is working\n";
            }
            
            int get_id() const { return id_; }
            
        private:
            int id_;
            static int counter_;
        };

        mutable std::mutex demo_mutex_;
    };

    /**
     * @brief Utility functions for RAII
     */
    namespace RAIIUtils {
        
        /**
         * @brief Create a temporary file with RAII cleanup
         */
        std::unique_ptr<FileRAII> create_temp_file(const std::string& content = "");

        /**
         * @brief Measure execution time of any callable
         */
        template<typename F>
        auto measure_execution(F&& func, const std::string& operation_name = "operation") {
            TimerRAII timer(operation_name);
            return func();
        }

        /**
         * @brief Create a scoped cleanup action
         */
        template<typename F>
        auto on_scope_exit(F&& cleanup) {
            return make_scope_guard(std::forward<F>(cleanup));
        }

        /**
         * @brief RAII wrapper for C-style arrays
         */
        template<typename T>
        class ArrayRAII {
        public:
            explicit ArrayRAII(size_t count) : array_(new T[count]), size_(count) {
                std::cout << "ArrayRAII: Allocated array of " << count 
                          << " elements of type " << typeid(T).name() << "\n";
            }

            ~ArrayRAII() {
                delete[] array_;
                std::cout << "ArrayRAII: Deallocated array of " << size_ << " elements\n";
            }

            // Non-copyable but movable
            ArrayRAII(const ArrayRAII&) = delete;
            ArrayRAII& operator=(const ArrayRAII&) = delete;

            ArrayRAII(ArrayRAII&& other) noexcept
                : array_(other.array_), size_(other.size_) {
                other.array_ = nullptr;
                other.size_ = 0;
            }

            ArrayRAII& operator=(ArrayRAII&& other) noexcept {
                if (this != &other) {
                    delete[] array_;
                    array_ = other.array_;
                    size_ = other.size_;
                    other.array_ = nullptr;
                    other.size_ = 0;
                }
                return *this;
            }

            T& operator[](size_t index) {
                assert(index < size_);
                return array_[index];
            }

            const T& operator[](size_t index) const {
                assert(index < size_);
                return array_[index];
            }

            T* data() const { return array_; }
            size_t size() const { return size_; }

        private:
            T* array_;
            size_t size_;
        };
    }

} // namespace CppVerseHub::Memory

#endif // RAII_EXAMPLES_HPP