// File: tests/unit_tests/memory_tests/RAIITests.cpp
// RAII pattern tests for CppVerseHub memory management showcase

#include <catch2/catch.hpp>
#include <memory>
#include <fstream>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <stdexcept>

// Include core classes and utilities
#include "MemoryTracker.hpp"
#include "FileManager.hpp"
#include "ResourceLock.hpp"

using namespace CppVerseHub::Utils;

/**
 * @brief Test fixture for RAII tests
 */
class RAIITestFixture {
public:
    RAIITestFixture() {
        MemoryTracker::resetCounters();
        cleanup_test_files();
    }
    
    ~RAIITestFixture() {
        cleanup_test_files();
        MemoryTracker::printMemoryStats("RAII test completion");
    }
    
private:
    void cleanup_test_files() {
        std::vector<std::string> testFiles = {
            "raii_test.txt",
            "raii_test_backup.txt",
            "raii_temp_file.txt",
            "raii_log.txt"
        };
        
        for (const std::string& filename : testFiles) {
            std::remove(filename.c_str());
        }
    }
};

/**
 * @brief RAII class for file operations
 */
class FileRAII {
private:
    std::string filename_;
    std::FILE* file_;
    bool ownsFile_;
    
public:
    explicit FileRAII(const std::string& filename, const std::string& mode = "w")
        : filename_(filename), ownsFile_(true) {
        file_ = std::fopen(filename.c_str(), mode.c_str());
        if (!file_) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
    }
    
    // Move constructor
    FileRAII(FileRAII&& other) noexcept
        : filename_(std::move(other.filename_)), file_(other.file_), ownsFile_(other.ownsFile_) {
        other.file_ = nullptr;
        other.ownsFile_ = false;
    }
    
    // Move assignment
    FileRAII& operator=(FileRAII&& other) noexcept {
        if (this != &other) {
            close();
            filename_ = std::move(other.filename_);
            file_ = other.file_;
            ownsFile_ = other.ownsFile_;
            other.file_ = nullptr;
            other.ownsFile_ = false;
        }
        return *this;
    }
    
    // Disable copy operations
    FileRAII(const FileRAII&) = delete;
    FileRAII& operator=(const FileRAII&) = delete;
    
    ~FileRAII() {
        close();
    }
    
    void write(const std::string& data) {
        if (file_ && ownsFile_) {
            std::fprintf(file_, "%s", data.c_str());
            std::fflush(file_);
        }
    }
    
    std::string read() {
        if (!file_ || !ownsFile_) return "";
        
        std::fseek(file_, 0, SEEK_END);
        long length = std::ftell(file_);
        std::fseek(file_, 0, SEEK_SET);
        
        if (length <= 0) return "";
        
        std::string content(length, '\0');
        std::fread(&content[0], 1, length, file_);
        return content;
    }
    
    void close() {
        if (file_ && ownsFile_) {
            std::fclose(file_);
        }
        file_ = nullptr;
        ownsFile_ = false;
    }
    
    bool isOpen() const {
        return file_ != nullptr && ownsFile_;
    }
    
    const std::string& getFilename() const {
        return filename_;
    }
};

/**
 * @brief RAII class for memory management
 */
template<typename T>
class MemoryRAII {
private:
    T* ptr_;
    size_t size_;
    bool ownsMemory_;
    
public:
    explicit MemoryRAII(size_t count)
        : ptr_(new T[count]), size_(count), ownsMemory_(true) {
        // Initialize memory to zero
        std::memset(ptr_, 0, sizeof(T) * count);
    }
    
    // Move constructor
    MemoryRAII(MemoryRAII&& other) noexcept
        : ptr_(other.ptr_), size_(other.size_), ownsMemory_(other.ownsMemory_) {
        other.ptr_ = nullptr;
        other.size_ = 0;
        other.ownsMemory_ = false;
    }
    
    // Move assignment
    MemoryRAII& operator=(MemoryRAII&& other) noexcept {
        if (this != &other) {
            cleanup();
            ptr_ = other.ptr_;
            size_ = other.size_;
            ownsMemory_ = other.ownsMemory_;
            other.ptr_ = nullptr;
            other.size_ = 0;
            other.ownsMemory_ = false;
        }
        return *this;
    }
    
    // Disable copy operations
    MemoryRAII(const MemoryRAII&) = delete;
    MemoryRAII& operator=(const MemoryRAII&) = delete;
    
    ~MemoryRAII() {
        cleanup();
    }
    
    T& operator[](size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return ptr_[index];
    }
    
    const T& operator[](size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return ptr_[index];
    }
    
    T* get() { return ptr_; }
    const T* get() const { return ptr_; }
    size_t size() const { return size_; }
    
    void fill(const T& value) {
        for (size_t i = 0; i < size_; ++i) {
            ptr_[i] = value;
        }
    }
    
private:
    void cleanup() {
        if (ptr_ && ownsMemory_) {
            delete[] ptr_;
        }
        ptr_ = nullptr;
        size_ = 0;
        ownsMemory_ = false;
    }
};

/**
 * @brief RAII class for mutex locks with timeout
 */
class TimedLockRAII {
private:
    std::mutex& mutex_;
    bool isLocked_;
    std::chrono::milliseconds timeout_;
    
public:
    explicit TimedLockRAII(std::mutex& mtx, std::chrono::milliseconds timeout = std::chrono::milliseconds(1000))
        : mutex_(mtx), timeout_(timeout) {
        isLocked_ = mutex_.try_lock_for(timeout_);
        if (!isLocked_) {
            throw std::runtime_error("Failed to acquire lock within timeout");
        }
    }
    
    ~TimedLockRAII() {
        if (isLocked_) {
            mutex_.unlock();
        }
    }
    
    // Non-copyable and non-movable for simplicity
    TimedLockRAII(const TimedLockRAII&) = delete;
    TimedLockRAII& operator=(const TimedLockRAII&) = delete;
    TimedLockRAII(TimedLockRAII&&) = delete;
    TimedLockRAII& operator=(TimedLockRAII&&) = delete;
    
    bool isLocked() const { return isLocked_; }
};

/**
 * @brief RAII class for resource counting
 */
class ResourceCounter {
private:
    static std::atomic<int> activeResources_;
    std::string name_;
    int id_;
    
public:
    explicit ResourceCounter(const std::string& name)
        : name_(name), id_(activeResources_.fetch_add(1) + 1) {}
    
    ~ResourceCounter() {
        activeResources_.fetch_sub(1);
    }
    
    // Move constructor
    ResourceCounter(ResourceCounter&& other) noexcept
        : name_(std::move(other.name_)), id_(other.id_) {
        other.id_ = -1; // Mark as moved
    }
    
    // Move assignment
    ResourceCounter& operator=(ResourceCounter&& other) noexcept {
        if (this != &other) {
            name_ = std::move(other.name_);
            id_ = other.id_;
            other.id_ = -1;
        }
        return *this;
    }
    
    // Disable copy operations
    ResourceCounter(const ResourceCounter&) = delete;
    ResourceCounter& operator=(const ResourceCounter&) = delete;
    
    const std::string& getName() const { return name_; }
    int getId() const { return id_; }
    
    static int getActiveCount() { return activeResources_.load(); }
    static void resetCounter() { activeResources_.store(0); }
};

std::atomic<int> ResourceCounter::activeResources_{0};

TEST_CASE_METHOD(RAIITestFixture, "Basic RAII Principles", "[raii][basic][principles]") {
    
    SECTION("Automatic resource cleanup") {
        const std::string testData = "RAII test data\nSecond line\n";
        std::string filename = "raii_test.txt";
        
        // Resource acquisition and automatic cleanup
        {
            FileRAII file(filename, "w");
            REQUIRE(file.isOpen());
            file.write(testData);
        } // File automatically closed here
        
        // Verify file was written and closed properly
        std::ifstream verifyFile(filename);
        REQUIRE(verifyFile.is_open());
        
        std::string content((std::istreambuf_iterator<char>(verifyFile)),
                           std::istreambuf_iterator<char>());
        REQUIRE(content == testData);
    }
    
    SECTION("Exception safety with RAII") {
        std::string filename = "raii_exception_test.txt";
        bool fileCreated = false;
        
        try {
            FileRAII file(filename, "w");
            fileCreated = file.isOpen();
            file.write("Before exception\n");
            
            // Simulate exception
            throw std::runtime_error("Test exception");
            
            file.write("After exception\n"); // This should not execute
        } catch (const std::runtime_error& e) {
            REQUIRE(std::string(e.what()) == "Test exception");
        }
        
        REQUIRE(fileCreated); // File was created
        
        // Verify file was properly closed despite exception
        std::ifstream verifyFile(filename);
        REQUIRE(verifyFile.is_open());
        
        std::string content((std::istreambuf_iterator<char>(verifyFile)),
                           std::istreambuf_iterator<char>());
        REQUIRE(content == "Before exception\n");
        
        std::remove(filename.c_str()); // Cleanup
    }
    
    SECTION("Move semantics with RAII") {
        std::string filename = "raii_move_test.txt";
        
        auto createFile = [&filename]() -> FileRAII {
            return FileRAII(filename, "w");
        };
        
        {
            FileRAII file = createFile(); // Move constructor
            REQUIRE(file.isOpen());
            file.write("Moved file content\n");
            
            FileRAII anotherFile(std::move(file)); // Move constructor
            REQUIRE(!file.isOpen());  // Original should be empty
            REQUIRE(anotherFile.isOpen()); // New should own resource
            
            anotherFile.write("Additional content\n");
        } // anotherFile destructor closes file
        
        // Verify content
        std::ifstream verifyFile(filename);
        std::string content((std::istreambuf_iterator<char>(verifyFile)),
                           std::istreambuf_iterator<char>());
        REQUIRE(content == "Moved file content\nAdditional content\n");
        
        std::remove(filename.c_str());
    }
}

TEST_CASE_METHOD(RAIITestFixture, "Memory Management RAII", "[raii][memory][management]") {
    
    SECTION("Automatic memory deallocation") {
        const size_t arraySize = 1000;
        
        {
            MemoryRAII<int> intArray(arraySize);
            REQUIRE(intArray.size() == arraySize);
            
            // Fill array with test data
            for (size_t i = 0; i < arraySize; ++i) {
                intArray[i] = static_cast<int>(i * 2);
            }
            
            // Verify data
            REQUIRE(intArray[0] == 0);
            REQUIRE(intArray[100] == 200);
            REQUIRE(intArray[arraySize - 1] == static_cast<int>((arraySize - 1) * 2));
            
        } // Memory automatically deallocated here
        
        // If we had a memory leak detector, it would show clean deallocation
    }
    
    SECTION("Exception safety with memory RAII") {
        const size_t arraySize = 500;
        bool memoryAllocated = false;
        
        try {
            MemoryRAII<double> doubleArray(arraySize);
            memoryAllocated = (doubleArray.get() != nullptr);
            
            // Fill with some data
            doubleArray.fill(3.14159);
            REQUIRE(doubleArray[100] == 3.14159);
            
            // Simulate exception during processing
            if (arraySize > 100) {
                throw std::logic_error("Processing error");
            }
            
        } catch (const std::logic_error& e) {
            REQUIRE(std::string(e.what()) == "Processing error");
        }
        
        REQUIRE(memoryAllocated); // Memory was allocated
        // Memory should be automatically freed despite exception
    }
    
    SECTION("Move semantics with memory RAII") {
        const size_t arraySize = 200;
        
        auto createArray = [arraySize]() -> MemoryRAII<int> {
            MemoryRAII<int> array(arraySize);
            array.fill(42);
            return array; // Move semantics
        };
        
        {
            MemoryRAII<int> myArray = createArray();
            REQUIRE(myArray.size() == arraySize);
            REQUIRE(myArray[50] == 42);
            
            // Move to another array
            MemoryRAII<int> anotherArray = std::move(myArray);
            REQUIRE(myArray.get() == nullptr);    // Moved from
            REQUIRE(anotherArray.get() != nullptr); // Moved to
            REQUIRE(anotherArray.size() == arraySize);
            REQUIRE(anotherArray[50] == 42);
            
        } // anotherArray destructor frees memory
    }
}

TEST_CASE_METHOD(RAIITestFixture, "Concurrency RAII", "[raii][concurrency][synchronization]") {
    
    SECTION("Automatic mutex unlocking") {
        std::mutex testMutex;
        int sharedValue = 0;
        const int threadCount = 10;
        const int incrementsPerThread = 100;
        
        std::vector<std::thread> threads;
        
        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back([&testMutex, &sharedValue, incrementsPerThread] {
                for (int j = 0; j < incrementsPerThread; ++j) {
                    std::lock_guard<std::mutex> lock(testMutex); // RAII lock
                    ++sharedValue;
                } // lock automatically released here
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        REQUIRE(sharedValue == threadCount * incrementsPerThread);
    }
    
    SECTION("Exception safety with locks") {
        std::mutex testMutex;
        std::vector<int> sharedData;
        std::atomic<int> exceptionsThrown{0};
        
        auto riskyOperation = [&testMutex, &sharedData, &exceptionsThrown](int value) {
            try {
                std::lock_guard<std::mutex> lock(testMutex); // RAII lock
                sharedData.push_back(value);
                
                if (value % 5 == 0) {
                    throw std::runtime_error("Simulated error");
                }
                
                sharedData.push_back(value * 2);
                
            } catch (const std::runtime_error&) {
                exceptionsThrown.fetch_add(1);
                // Lock automatically released despite exception
            }
        };
        
        std::vector<std::thread> threads;
        
        for (int i = 0; i < 20; ++i) {
            threads.emplace_back([&riskyOperation, i] {
                riskyOperation(i);
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        REQUIRE(exceptionsThrown.load() > 0); // Some exceptions thrown
        REQUIRE(!sharedData.empty());          // Some operations succeeded
        
        // All locks should have been properly released
        bool canLock = testMutex.try_lock();
        REQUIRE(canLock);
        if (canLock) {
            testMutex.unlock();
        }
    }
    
    SECTION("Timed lock RAII") {
        std::timed_mutex timedMutex;
        std::atomic<int> successfulLocks{0};
        std::atomic<int> timeoutLocks{0};
        
        // Hold the mutex for a short time
        std::thread holder([&timedMutex] {
            TimedLockRAII lock(timedMutex, std::chrono::milliseconds(50));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        });
        
        // Try to acquire locks with timeout
        std::vector<std::thread> waiters;
        const int waiterCount = 5;
        
        for (int i = 0; i < waiterCount; ++i) {
            waiters.emplace_back([&timedMutex, &successfulLocks, &timeoutLocks] {
                try {
                    TimedLockRAII lock(timedMutex, std::chrono::milliseconds(25));
                    successfulLocks.fetch_add(1);
                } catch (const std::runtime_error&) {
                    timeoutLocks.fetch_add(1);
                }
            });
        }
        
        holder.join();
        for (auto& waiter : waiters) {
            waiter.join();
        }
        
        REQUIRE(successfulLocks.load() + timeoutLocks.load() == waiterCount);
        REQUIRE(timeoutLocks.load() > 0); // Some should timeout
    }
}

TEST_CASE_METHOD(RAIITestFixture, "Resource Counting RAII", "[raii][resources][counting]") {
    
    SECTION("Automatic resource counting") {
        ResourceCounter::resetCounter();
        
        {
            ResourceCounter resource1("Resource1");
            REQUIRE(ResourceCounter::getActiveCount() == 1);
            
            {
                ResourceCounter resource2("Resource2");
                ResourceCounter resource3("Resource3");
                REQUIRE(ResourceCounter::getActiveCount() == 3);
            } // resource2 and resource3 destroyed
            
            REQUIRE(ResourceCounter::getActiveCount() == 1);
        } // resource1 destroyed
        
        REQUIRE(ResourceCounter::getActiveCount() == 0);
    }
    
    SECTION("Move semantics with resource counting") {
        ResourceCounter::resetCounter();
        
        auto createResource = [](const std::string& name) -> ResourceCounter {
            return ResourceCounter(name);
        };
        
        {
            ResourceCounter res1 = createResource("MovedResource");
            REQUIRE(ResourceCounter::getActiveCount() == 1);
            REQUIRE(res1.getName() == "MovedResource");
            
            ResourceCounter res2 = std::move(res1);
            REQUIRE(ResourceCounter::getActiveCount() == 1); // Same count after move
            REQUIRE(res2.getName() == "MovedResource");
            
        } // res2 destroyed, res1 was moved from
        
        REQUIRE(ResourceCounter::getActiveCount() == 0);
    }
    
    SECTION("Exception safety with resource counting") {
        ResourceCounter::resetCounter();
        std::atomic<int> exceptionsHandled{0};
        
        auto riskyResourceCreation = [&exceptionsHandled](int id) {
            try {
                ResourceCounter resource("RiskyResource_" + std::to_string(id));
                
                if (id % 3 == 0) {
                    throw std::runtime_error("Resource creation failed");
                }
                
                // Do some work with resource
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                
            } catch (const std::runtime_error&) {
                exceptionsHandled.fetch_add(1);
                // Resource automatically cleaned up
            }
        };
        
        std::vector<std::thread> threads;
        
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&riskyResourceCreation, i] {
                riskyResourceCreation(i);
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        REQUIRE(exceptionsHandled.load() > 0); // Some exceptions occurred
        REQUIRE(ResourceCounter::getActiveCount() == 0); // All resources cleaned up
    }
}

TEST_CASE_METHOD(RAIITestFixture, "Complex RAII Scenarios", "[raii][complex][scenarios]") {
    
    SECTION("Nested RAII objects") {
        std::string outerFile = "raii_outer.txt";
        std::string innerFile = "raii_inner.txt";
        
        {
            FileRAII outer(outerFile, "w");
            outer.write("Outer file start\n");
            
            {
                FileRAII inner(innerFile, "w");
                inner.write("Inner file content\n");
                
                outer.write("After inner file creation\n");
                
                {
                    MemoryRAII<char> buffer(256);
                    buffer.fill('X');
                    
                    // Convert buffer content to string for writing
                    std::string bufferStr(buffer.get(), buffer.size());
                    outer.write("Buffer: " + bufferStr.substr(0, 10) + "...\n");
                    
                } // buffer destroyed
                
                outer.write("After buffer destruction\n");
                
            } // inner file closed
            
            outer.write("Outer file end\n");
            
        } // outer file closed
        
        // Verify both files were written correctly
        std::ifstream outerCheck(outerFile);
        std::string outerContent((std::istreambuf_iterator<char>(outerCheck)),
                                std::istreambuf_iterator<char>());
        
        std::ifstream innerCheck(innerFile);
        std::string innerContent((std::istreambuf_iterator<char>(innerCheck)),
                                std::istreambuf_iterator<char>());
        
        REQUIRE(outerContent.find("Outer file start") != std::string::npos);
        REQUIRE(outerContent.find("After inner file creation") != std::string::npos);
        REQUIRE(outerContent.find("Buffer: XXXXXXXXXX...") != std::string::npos);
        REQUIRE(outerContent.find("Outer file end") != std::string::npos);
        
        REQUIRE(innerContent == "Inner file content\n");
        
        std::remove(outerFile.c_str());
        std::remove(innerFile.c_str());
    }
    
    SECTION("RAII with early return") {
        auto processFile = [](const std::string& filename, bool shouldFail) -> std::string {
            FileRAII file(filename, "w");
            file.write("Processing started\n");
            
            if (shouldFail) {
                file.write("Error condition detected\n");
                return "FAILED"; // Early return - file should still be closed properly
            }
            
            file.write("Processing completed successfully\n");
            return "SUCCESS";
        };
        
        std::string successFile = "raii_success.txt";
        std::string failFile = "raii_fail.txt";
        
        std::string result1 = processFile(successFile, false);
        std::string result2 = processFile(failFile, true);
        
        REQUIRE(result1 == "SUCCESS");
        REQUIRE(result2 == "FAILED");
        
        // Both files should exist and be properly closed
        std::ifstream successCheck(successFile);
        std::ifstream failCheck(failFile);
        
        REQUIRE(successCheck.is_open());
        REQUIRE(failCheck.is_open());
        
        std::string successContent((std::istreambuf_iterator<char>(successCheck)),
                                  std::istreambuf_iterator<char>());
        std::string failContent((std::istreambuf_iterator<char>(failCheck)),
                               std::istreambuf_iterator<char>());
        
        REQUIRE(successContent.find("Processing completed successfully") != std::string::npos);
        REQUIRE(failContent.find("Error condition detected") != std::string::npos);
        REQUIRE(failContent.find("Processing completed successfully") == std::string::npos);
        
        std::remove(successFile.c_str());
        std::remove(failFile.c_str());
    }
    
    SECTION("RAII with multiple exception types") {
        class MultiExceptionRAII {
        private:
            std::string name_;
            static std::atomic<int> instanceCount_;
            
        public:
            explicit MultiExceptionRAII(const std::string& name) : name_(name) {
                instanceCount_.fetch_add(1);
                
                if (name.find("logic_error") != std::string::npos) {
                    throw std::logic_error("Logic error in constructor");
                }
                if (name.find("runtime_error") != std::string::npos) {
                    throw std::runtime_error("Runtime error in constructor");
                }
            }
            
            ~MultiExceptionRAII() {
                instanceCount_.fetch_sub(1);
            }
            
            MultiExceptionRAII(const MultiExceptionRAII&) = delete;
            MultiExceptionRAII& operator=(const MultiExceptionRAII&) = delete;
            
            MultiExceptionRAII(MultiExceptionRAII&& other) noexcept
                : name_(std::move(other.name_)) {
                // Don't increment counter for move
            }
            
            const std::string& getName() const { return name_; }
            
            static int getInstanceCount() { return instanceCount_.load(); }
            static void resetCounter() { instanceCount_.store(0); }
        };
        
        static std::atomic<int> MultiExceptionRAII::instanceCount_{0};
        MultiExceptionRAII::resetCounter();
        
        std::atomic<int> logicErrors{0};
        std::atomic<int> runtimeErrors{0};
        std::atomic<int> successes{0};
        
        std::vector<std::string> testNames = {
            "success_1",
            "logic_error_1",
            "runtime_error_1",
            "success_2",
            "logic_error_2",
            "success_3"
        };
        
        for (const auto& name : testNames) {
            try {
                MultiExceptionRAII obj(name);
                successes.fetch_add(1);
            } catch (const std::logic_error&) {
                logicErrors.fetch_add(1);
            } catch (const std::runtime_error&) {
                runtimeErrors.fetch_add(1);
            }
        }
        
        REQUIRE(successes.load() == 3);
        REQUIRE(logicErrors.load() == 2);
        REQUIRE(runtimeErrors.load() == 1);
        REQUIRE(MultiExceptionRAII::getInstanceCount() == 0); // All cleaned up
    }
}

TEST_CASE_METHOD(RAIITestFixture, "RAII Performance and Best Practices", "[raii][performance][best-practices]") {
    
    SECTION("RAII overhead measurement") {
        const int iterations = 100000;
        
        // Measure RAII overhead
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            MemoryRAII<int> array(100);
            array.fill(i);
            // Automatic cleanup
        }
        
        auto raiiTime = std::chrono::high_resolution_clock::now() - start;
        
        // Measure manual management for comparison
        start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            int* array = new int[100];
            std::fill(array, array + 100, i);
            delete[] array; // Manual cleanup
        }
        
        auto manualTime = std::chrono::high_resolution_clock::now() - start;
        
        auto raiiMs = std::chrono::duration_cast<std::chrono::milliseconds>(raiiTime).count();
        auto manualMs = std::chrono::duration_cast<std::chrono::milliseconds>(manualTime).count();
        
        INFO("RAII time: " << raiiMs << "ms");
        INFO("Manual time: " << manualMs << "ms");
        
        // RAII should have minimal overhead
        REQUIRE(raiiMs < manualMs * 2); // Should not be more than 2x slower
    }
    
    SECTION("Stack vs heap RAII comparison") {
        const int iterations = 50000;
        
        // Stack-based RAII
        auto stackStart = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            struct StackRAII {
                int data[100];
                StackRAII() { std::fill(data, data + 100, 42); }
            } stackObj;
            
            volatile int sum = 0;
            for (int j = 0; j < 100; ++j) {
                sum += stackObj.data[j];
            }
        }
        
        auto stackTime = std::chrono::high_resolution_clock::now() - stackStart;
        
        // Heap-based RAII
        auto heapStart = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            MemoryRAII<int> heapObj(100);
            heapObj.fill(42);
            
            volatile int sum = 0;
            for (size_t j = 0; j < heapObj.size(); ++j) {
                sum += heapObj[j];
            }
        }
        
        auto heapTime = std::chrono::high_resolution_clock::now() - heapStart;
        
        auto stackMs = std::chrono::duration_cast<std::chrono::milliseconds>(stackTime).count();
        auto heapMs = std::chrono::duration_cast<std::chrono::milliseconds>(heapTime).count();
        
        INFO("Stack RAII time: " << stackMs << "ms");
        INFO("Heap RAII time: " << heapMs << "ms");
        
        // Stack should generally be faster
        REQUIRE(stackMs <= heapMs);
    }
    
    SECTION("RAII move semantics efficiency") {
        const int iterations = 10000;
        
        auto createLargeObject = []() -> MemoryRAII<double> {
            MemoryRAII<double> obj(1000);
            obj.fill(3.14159);
            return obj; // Move semantics
        };
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<MemoryRAII<double>> objects;
        for (int i = 0; i < iterations; ++i) {
            objects.push_back(createLargeObject()); // Move construction
        }
        
        auto moveTime = std::chrono::high_resolution_clock::now() - start;
        
        auto moveMs = std::chrono::duration_cast<std::chrono::milliseconds>(moveTime).count();
        
        INFO("Move semantics time for " << iterations << " objects: " << moveMs << "ms");
        
        REQUIRE(objects.size() == iterations);
        REQUIRE(moveMs < 5000); // Should complete in reasonable time
        
        // Verify objects were moved correctly
        REQUIRE(objects[0].size() == 1000);
        REQUIRE(objects[iterations-1][500] == 3.14159);
    }
}

TEST_CASE_METHOD(RAIITestFixture, "Real-World RAII Applications", "[raii][real-world][applications]") {
    
    SECTION("Database connection RAII simulation") {
        class DatabaseConnectionRAII {
        private:
            std::string connectionString_;
            bool isConnected_;
            static std::atomic<int> activeConnections_;
            
        public:
            explicit DatabaseConnectionRAII(const std::string& connStr)
                : connectionString_(connStr), isConnected_(false) {
                // Simulate connection establishment
                isConnected_ = true;
                activeConnections_.fetch_add(1);
            }
            
            ~DatabaseConnectionRAII() {
                if (isConnected_) {
                    // Simulate connection cleanup
                    activeConnections_.fetch_sub(1);
                    isConnected_ = false;
                }
            }
            
            // Non-copyable
            DatabaseConnectionRAII(const DatabaseConnectionRAII&) = delete;
            DatabaseConnectionRAII& operator=(const DatabaseConnectionRAII&) = delete;
            
            // Movable
            DatabaseConnectionRAII(DatabaseConnectionRAII&& other) noexcept
                : connectionString_(std::move(other.connectionString_)), isConnected_(other.isConnected_) {
                other.isConnected_ = false;
            }
            
            bool executeQuery(const std::string& query) {
                if (!isConnected_) return false;
                
                // Simulate query execution
                if (query.find("FAIL") != std::string::npos) {
                    throw std::runtime_error("Query execution failed");
                }
                
                return true;
            }
            
            bool isConnected() const { return isConnected_; }
            
            static int getActiveConnections() { return activeConnections_.load(); }
            static void resetConnections() { activeConnections_.store(0); }
        };
        
        static std::atomic<int> DatabaseConnectionRAII::activeConnections_{0};
        DatabaseConnectionRAII::resetConnections();
        
        std::atomic<int> successfulQueries{0};
        std::atomic<int> failedQueries{0};
        
        std::vector<std::thread> workers;
        const int workerCount = 5;
        
        for (int i = 0; i < workerCount; ++i) {
            workers.emplace_back([&successfulQueries, &failedQueries, i] {
                try {
                    DatabaseConnectionRAII conn("database://server" + std::to_string(i));
                    
                    // Execute multiple queries
                    std::vector<std::string> queries = {
                        "SELECT * FROM planets",
                        "UPDATE fleets SET position = ?",
                        i % 3 == 0 ? "SELECT FAIL FROM invalid" : "SELECT * FROM missions"
                    };
                    
                    for (const auto& query : queries) {
                        try {
                            if (conn.executeQuery(query)) {
                                successfulQueries.fetch_add(1);
                            }
                        } catch (const std::runtime_error&) {
                            failedQueries.fetch_add(1);
                        }
                    }
                    
                } catch (...) {
                    // Connection establishment failed
                }
                // Connection automatically closed here
            });
        }
        
        for (auto& worker : workers) {
            worker.join();
        }
        
        REQUIRE(successfulQueries.load() > 0);
        REQUIRE(failedQueries.load() > 0);
        REQUIRE(DatabaseConnectionRAII::getActiveConnections() == 0); // All connections closed
    }
    
    SECTION("Network resource RAII simulation") {
        class NetworkResourceRAII {
        private:
            std::string endpoint_;
            int sockfd_;
            bool isConnected_;
            
        public:
            explicit NetworkResourceRAII(const std::string& endpoint)
                : endpoint_(endpoint), sockfd_(-1), isConnected_(false) {
                // Simulate socket creation and connection
                sockfd_ = rand() % 1000 + 1000; // Fake socket descriptor
                isConnected_ = true;
            }
            
            ~NetworkResourceRAII() {
                if (isConnected_ && sockfd_ != -1) {
                    // Simulate socket cleanup
                    sockfd_ = -1;
                    isConnected_ = false;
                }
            }
            
            // Non-copyable
            NetworkResourceRAII(const NetworkResourceRAII&) = delete;
            NetworkResourceRAII& operator=(const NetworkResourceRAII&) = delete;
            
            bool sendData(const std::string& data) {
                if (!isConnected_) return false;
                
                // Simulate network send
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                return true;
            }
            
            std::string receiveData() {
                if (!isConnected_) return "";
                
                // Simulate network receive
                return "Response from " + endpoint_;
            }
            
            bool isConnected() const { return isConnected_; }
            int getSocket() const { return sockfd_; }
        };
        
        std::vector<std::string> endpoints = {
            "api.game.com",
            "stats.server.net",
            "chat.service.org"
        };
        
        std::vector<std::thread> networkThreads;
        std::atomic<int> successfulOperations{0};
        
        for (const auto& endpoint : endpoints) {
            networkThreads.emplace_back([&endpoint, &successfulOperations] {
                try {
                    NetworkResourceRAII network(endpoint);
                    
                    if (network.sendData("Test data")) {
                        std::string response = network.receiveData();
                        if (!response.empty()) {
                            successfulOperations.fetch_add(1);
                        }
                    }
                    
                } catch (...) {
                    // Network operation failed
                }
                // Network resource automatically cleaned up
            });
        }
        
        for (auto& thread : networkThreads) {
            thread.join();
        }
        
        REQUIRE(successfulOperations.load() == endpoints.size());
    }
}