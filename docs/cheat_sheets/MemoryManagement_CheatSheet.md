# Memory Management Cheat Sheet

**Location:** `CppVerseHub/docs/cheat_sheets/MemoryManagement_CheatSheet.md`

## 📚 Modern C++ Memory Management Guide

### 🧠 Memory Fundamentals

#### **Memory Layout**

```cpp
#include <iostream>

// Stack vs Heap demonstration
void memory_layout_demo() {
    // Stack allocation - automatic storage
    int stack_variable = 42;                    // Lives on stack
    char stack_array[100];                      // Fixed size, stack

    // Heap allocation - dynamic storage (avoid raw pointers!)
    int* heap_variable = new int(42);           // Manual management needed
    int* heap_array = new int[100];             // Manual cleanup required

    // Modern approach - smart pointers
    auto smart_variable = std::make_unique<int>(42);        // Automatic cleanup
    auto smart_array = std::make_unique<int[]>(100);        // Automatic cleanup

    std::cout << "Stack variable address: " << &stack_variable << "\n";
    std::cout << "Heap variable address: " << heap_variable << "\n";
    std::cout << "Smart pointer address: " << smart_variable.get() << "\n";

    // Manual cleanup (bad practice - use smart pointers instead!)
    delete heap_variable;
    delete[] heap_array;

    // Smart pointers clean up automatically when going out of scope
}

// Memory regions
class MemoryRegions {
public:
    static int static_variable;          // Data segment (initialized)
    static int uninitialized_static;     // BSS segment (uninitialized)

    void demonstrate_regions() {
        int local_var = 10;              // Stack

        static int local_static = 20;    // Data segment

        auto heap_ptr = std::make_unique<int>(30);  // Heap

        const char* literal = "Hello";   // Text segment (read-only)

        std::cout << "Addresses:\n";
        std::cout << "Static: " << &static_variable << "\n";
        std::cout << "Local: " << &local_var << "\n";
        std::cout << "Heap: " << heap_ptr.get() << "\n";
        std::cout << "Literal: " << static_cast<const void*>(literal) << "\n";
    }
};

int MemoryRegions::static_variable = 100;
int MemoryRegions::uninitialized_static;
```

### 🔧 Smart Pointers

#### **std::unique_ptr - Exclusive Ownership**

```cpp
#include <memory>

class Planet {
private:
    std::string name_;
    double resources_;

public:
    Planet(const std::string& name, double resources)
        : name_(name), resources_(resources) {
        std::cout << "Planet " << name_ << " created\n";
    }

    ~Planet() {
        std::cout << "Planet " << name_ << " destroyed\n";
    }

    const std::string& getName() const { return name_; }
    double getResources() const { return resources_; }

    void addResources(double amount) { resources_ += amount; }
};

void unique_ptr_examples() {
    // Creation methods
    auto planet1 = std::make_unique<Planet>("Earth", 1000.0);          // Preferred
    std::unique_ptr<Planet> planet2(new Planet("Mars", 500.0));        // Less safe

    // Access methods
    std::cout << "Planet name: " << planet1->getName() << "\n";        // Arrow operator
    std::cout << "Resources: " << (*planet1).getResources() << "\n";   // Dereference

    // Raw pointer access (when needed for C APIs)
    Planet* raw_ptr = planet1.get();  // Don't delete this!

    // Transfer ownership
    std::unique_ptr<Planet> planet3 = std::move(planet1);  // planet1 is now nullptr

    if (!planet1) {
        std::cout << "planet1 is now null after move\n";
    }

    // Reset and release
    planet3.reset();  // Destroys the managed object

    auto planet4 = std::make_unique<Planet>("Venus", 800.0);
    Planet* released = planet4.release();  // Returns raw pointer, transfers ownership
    delete released;  // Must manually delete now (not recommended)

    // Custom deleter
    auto custom_deleter = [](Planet* p) {
        std::cout << "Custom deleter for " << p->getName() << "\n";
        delete p;
    };

    std::unique_ptr<Planet, decltype(custom_deleter)>
        planet5(new Planet("Jupiter", 2000.0), custom_deleter);
}

// Using unique_ptr with arrays
void unique_ptr_arrays() {
    // Array version
    auto int_array = std::make_unique<int[]>(10);  // Array of 10 ints

    // Initialize array
    for (int i = 0; i < 10; ++i) {
        int_array[i] = i * i;
    }

    // Array of objects
    auto planet_array = std::make_unique<Planet[]>(3);
    // Note: This calls default constructor, so Planet needs one

    // Better approach for object arrays
    std::vector<std::unique_ptr<Planet>> planet_vector;
    planet_vector.push_back(std::make_unique<Planet>("Mercury", 100.0));
    planet_vector.push_back(std::make_unique<Planet>("Venus", 200.0));
    planet_vector.push_back(std::make_unique<Planet>("Earth", 300.0));
}
```

#### **std::shared_ptr - Shared Ownership**

```cpp
class Fleet {
private:
    std::string name_;
    std::shared_ptr<Planet> home_planet_;  // Shared ownership

public:
    Fleet(const std::string& name, std::shared_ptr<Planet> home)
        : name_(name), home_planet_(home) {
        std::cout << "Fleet " << name_ << " created\n";
    }

    ~Fleet() {
        std::cout << "Fleet " << name_ << " destroyed\n";
    }

    const std::string& getName() const { return name_; }
    std::shared_ptr<Planet> getHomePlanet() const { return home_planet_; }
};

void shared_ptr_examples() {
    // Creation methods
    auto earth = std::make_shared<Planet>("Earth", 1000.0);        // Preferred - single allocation
    std::shared_ptr<Planet> mars(new Planet("Mars", 500.0));       // Two allocations

    std::cout << "Earth reference count: " << earth.use_count() << "\n";  // Should be 1

    // Multiple owners
    auto fleet1 = std::make_shared<Fleet>("Alpha Squadron", earth);
    auto fleet2 = std::make_shared<Fleet>("Beta Squadron", earth);

    std::cout << "Earth reference count: " << earth.use_count() << "\n";  // Should be 3

    // Copying increases reference count
    std::shared_ptr<Planet> earth_copy = earth;
    std::cout << "After copy: " << earth.use_count() << "\n";  // Should be 4

    // Check if unique
    if (!earth.unique()) {
        std::cout << "Earth is shared among multiple owners\n";
    }

    // Manual reset
    earth_copy.reset();
    std::cout << "After reset: " << earth.use_count() << "\n";  // Should be 3 again

    // Aliasing constructor (sharing ownership but pointing to different object)
    auto planet_name = std::shared_ptr<std::string>(earth, &earth->getName());
    std::cout << "Planet name shared_ptr: " << *planet_name << "\n";
    std::cout << "Earth reference count: " << earth.use_count() << "\n";  // Still counts earth
}

// Thread-safe reference counting
void thread_safe_shared_ptr() {
    auto shared_planet = std::make_shared<Planet>("ThreadSafe", 1000.0);

    std::vector<std::thread> threads;

    // Create multiple threads that share the planet
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([shared_planet, i]() {
            // Each thread gets a copy of shared_ptr
            auto local_copy = shared_planet;  // Thread-safe copy

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "Thread " << i << " accessing: " << local_copy->getName() << "\n";

            // Reference counting is thread-safe
            // Object destruction is thread-safe
            // But object access is NOT automatically thread-safe!
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }
}
```

#### **std::weak_ptr - Non-owning Observer**

```cpp
class Mission;

class MissionObserver {
private:
    std::weak_ptr<Mission> observed_mission_;  // Won't prevent destruction
    std::string observer_name_;

public:
    MissionObserver(const std::string& name) : observer_name_(name) {}

    void observeMission(std::shared_ptr<Mission> mission) {
        observed_mission_ = mission;  // Convert to weak_ptr automatically
    }

    void checkMission() {
        if (auto mission = observed_mission_.lock()) {  // Try to get shared_ptr
            std::cout << observer_name_ << ": Mission still active\n";
            // Use mission safely here
        } else {
            std::cout << observer_name_ << ": Mission has been destroyed\n";
        }
    }

    bool isMissionAlive() const {
        return !observed_mission_.expired();
    }
};

class Mission {
private:
    std::string name_;
    std::vector<std::weak_ptr<MissionObserver>> observers_;  // Break circular reference

public:
    Mission(const std::string& name) : name_(name) {
        std::cout << "Mission " << name_ << " started\n";
    }

    ~Mission() {
        std::cout << "Mission " << name_ << " completed\n";
    }

    void addObserver(std::shared_ptr<MissionObserver> observer) {
        observers_.push_back(observer);  // Store as weak_ptr
        observer->observeMission(shared_from_this());
    }

    void notifyObservers() {
        // Clean up expired observers while notifying
        observers_.erase(
            std::remove_if(observers_.begin(), observers_.end(),
                [](const std::weak_ptr<MissionObserver>& weak_obs) {
                    if (auto obs = weak_obs.lock()) {
                        obs->checkMission();
                        return false;  // Keep valid observers
                    }
                    return true;  // Remove expired observers
                }),
            observers_.end()
        );
    }

    const std::string& getName() const { return name_; }
};

void weak_ptr_examples() {
    auto observer1 = std::make_shared<MissionObserver>("Observer1");
    auto observer2 = std::make_shared<MissionObserver>("Observer2");

    {
        auto mission = std::make_shared<Mission>("Exploration Alpha");

        mission->addObserver(observer1);
        mission->addObserver(observer2);

        std::cout << "\n--- Mission active ---\n";
        observer1->checkMission();  // Should find mission alive
        observer2->checkMission();  // Should find mission alive

        mission->notifyObservers();

    }  // mission goes out of scope and is destroyed

    std::cout << "\n--- Mission destroyed ---\n";
    observer1->checkMission();  // Should report mission destroyed
    observer2->checkMission();  // Should report mission destroyed
}

// Breaking circular references
class Planet;
class Colony {
    std::shared_ptr<Planet> planet_;      // Colony owns reference to planet
    std::string name_;

public:
    Colony(const std::string& name, std::shared_ptr<Planet> planet)
        : name_(name), planet_(planet) {}

    std::shared_ptr<Planet> getPlanet() const { return planet_; }
    const std::string& getName() const { return name_; }
};

class PlanetWithColonies {
private:
    std::string name_;
    std::vector<std::weak_ptr<Colony>> colonies_;  // Use weak_ptr to avoid cycles

public:
    PlanetWithColonies(const std::string& name) : name_(name) {}

    void addColony(std::shared_ptr<Colony> colony) {
        colonies_.push_back(colony);  // Automatic conversion to weak_ptr
    }

    void listActiveColonies() {
        std::cout << "Active colonies on " << name_ << ":\n";
        for (auto it = colonies_.begin(); it != colonies_.end();) {
            if (auto colony = it->lock()) {
                std::cout << "  - " << colony->getName() << "\n";
                ++it;
            } else {
                it = colonies_.erase(it);  // Remove expired colonies
            }
        }
    }

    const std::string& getName() const { return name_; }
};
```

### 🏗️ RAII (Resource Acquisition Is Initialization)

#### **Basic RAII Principles**

```cpp
// RAII wrapper for file handling
class FileWrapper {
private:
    FILE* file_;
    std::string filename_;

public:
    FileWrapper(const std::string& filename, const char* mode)
        : filename_(filename), file_(fopen(filename.c_str(), mode)) {
        if (!file_) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
        std::cout << "File " << filename_ << " opened\n";
    }

    ~FileWrapper() {
        if (file_) {
            fclose(file_);
            std::cout << "File " << filename_ << " closed\n";
        }
    }

    // Delete copy operations to prevent double-close
    FileWrapper(const FileWrapper&) = delete;
    FileWrapper& operator=(const FileWrapper&) = delete;

    // Move operations
    FileWrapper(FileWrapper&& other) noexcept
        : file_(other.file_), filename_(std::move(other.filename_)) {
        other.file_ = nullptr;
    }

    FileWrapper& operator=(FileWrapper&& other) noexcept {
        if (this != &other) {
            if (file_) fclose(file_);

            file_ = other.file_;
            filename_ = std::move(other.filename_);
            other.file_ = nullptr;
        }
        return *this;
    }

    FILE* get() const { return file_; }

    void write(const std::string& data) {
        if (file_) {
            fprintf(file_, "%s", data.c_str());
        }
    }
};

// RAII for mutex locking
class TimedLock {
private:
    std::mutex& mutex_;
    bool locked_;

public:
    TimedLock(std::mutex& m, std::chrono::milliseconds timeout)
        : mutex_(m), locked_(false) {

        auto start = std::chrono::steady_clock::now();

        while (!locked_ &&
               (std::chrono::steady_clock::now() - start) < timeout) {
            if (mutex_.try_lock()) {
                locked_ = true;
                std::cout << "Lock acquired\n";
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        if (!locked_) {
            throw std::runtime_error("Failed to acquire lock within timeout");
        }
    }

    ~TimedLock() {
        if (locked_) {
            mutex_.unlock();
            std::cout << "Lock released\n";
        }
    }

    TimedLock(const TimedLock&) = delete;
    TimedLock& operator=(const TimedLock&) = delete;

    bool isLocked() const { return locked_; }
};

// RAII for network connections
class NetworkConnection {
private:
    std::string server_;
    int port_;
    bool connected_;

    void connect() {
        // Simulate connection
        std::cout << "Connecting to " << server_ << ":" << port_ << "\n";
        connected_ = true;
    }

    void disconnect() {
        if (connected_) {
            std::cout << "Disconnecting from " << server_ << ":" << port_ << "\n";
            connected_ = false;
        }
    }

public:
    NetworkConnection(const std::string& server, int port)
        : server_(server), port_(port), connected_(false) {
        connect();
    }

    ~NetworkConnection() {
        disconnect();
    }

    // Non-copyable, movable
    NetworkConnection(const NetworkConnection&) = delete;
    NetworkConnection& operator=(const NetworkConnection&) = delete;

    NetworkConnection(NetworkConnection&& other) noexcept
        : server_(std::move(other.server_)), port_(other.port_), connected_(other.connected_) {
        other.connected_ = false;
    }

    void sendData(const std::string& data) {
        if (connected_) {
            std::cout << "Sending: " << data << "\n";
        }
    }

    bool isConnected() const { return connected_; }
};

void raii_examples() {
    try {
        // File automatically closed even if exception occurs
        FileWrapper file("test.txt", "w");
        file.write("Hello RAII!\n");

        // Network automatically disconnected
        NetworkConnection conn("localhost", 8080);
        conn.sendData("Test message");

        // Exception-safe mutex locking
        std::mutex test_mutex;
        {
            TimedLock lock(test_mutex, std::chrono::milliseconds(100));
            // Critical section
            std::cout << "In critical section\n";
            // Lock automatically released when leaving scope
        }

    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << "\n";
        // All resources still cleaned up properly
    }
}
```

#### **RAII with Custom Deleter**

```cpp
// Custom resource manager
template<typename Resource, typename Deleter>
class RAIIWrapper {
private:
    Resource resource_;
    Deleter deleter_;
    bool owns_resource_;

public:
    RAIIWrapper(Resource resource, Deleter deleter)
        : resource_(resource), deleter_(deleter), owns_resource_(true) {}

    ~RAIIWrapper() {
        if (owns_resource_) {
            deleter_(resource_);
        }
    }

    // Move-only
    RAIIWrapper(const RAIIWrapper&) = delete;
    RAIIWrapper& operator=(const RAIIWrapper&) = delete;

    RAIIWrapper(RAIIWrapper&& other) noexcept
        : resource_(other.resource_), deleter_(std::move(other.deleter_)), owns_resource_(other.owns_resource_) {
        other.owns_resource_ = false;
    }

    RAIIWrapper& operator=(RAIIWrapper&& other) noexcept {
        if (this != &other) {
            if (owns_resource_) {
                deleter_(resource_);
            }

            resource_ = other.resource_;
            deleter_ = std::move(other.deleter_);
            owns_resource_ = other.owns_resource_;
            other.owns_resource_ = false;
        }
        return *this;
    }

    Resource get() const { return resource_; }
    Resource release() {
        owns_resource_ = false;
        return resource_;
    }
};

// Helper function to create RAII wrapper
template<typename Resource, typename Deleter>
auto make_raii(Resource resource, Deleter deleter) {
    return RAIIWrapper<Resource, Deleter>(resource, deleter);
}

void custom_raii_examples() {
    // SDL texture example
    auto texture_deleter = [](void* texture) {
        std::cout << "SDL texture destroyed\n";
        // SDL_DestroyTexture((SDL_Texture*)texture);
    };

    void* fake_texture = malloc(100);  // Simulate SDL texture
    auto texture_wrapper = make_raii(fake_texture, [](void* ptr) {
        std::cout << "Custom texture cleanup\n";
        free(ptr);
    });

    // OpenGL buffer example
    unsigned int buffer_id = 12345;
    auto gl_buffer = make_raii(buffer_id, [](unsigned int id) {
        std::cout << "OpenGL buffer " << id << " deleted\n";
        // glDeleteBuffers(1, &id);
    });

    // Array with custom cleanup
    int* array = new int[100];
    auto array_wrapper = make_raii(array, [](int* arr) {
        std::cout << "Array deleted\n";
        delete[] arr;
    });

    std::cout << "Resources will be cleaned up automatically\n";
}
```

### 🏭 Custom Memory Allocators

#### **Basic Allocator Interface**

```cpp
template<typename T>
class BasicAllocator {
public:
    using value_type = T;

    BasicAllocator() = default;

    template<typename U>
    BasicAllocator(const BasicAllocator<U>&) {}

    T* allocate(size_t n) {
        if (n > std::numeric_limits<size_t>::max() / sizeof(T)) {
            throw std::bad_alloc();
        }

        T* ptr = static_cast<T*>(std::malloc(n * sizeof(T)));
        if (!ptr) {
            throw std::bad_alloc();
        }

        std::cout << "Allocated " << n << " objects of size " << sizeof(T) << "\n";
        return ptr;
    }

    void deallocate(T* ptr, size_t n) {
        std::cout << "Deallocated " << n << " objects\n";
        std::free(ptr);
    }

    template<typename U>
    bool operator==(const BasicAllocator<U>&) const { return true; }

    template<typename U>
    bool operator!=(const BasicAllocator<U>&) const { return false; }
};

// Memory pool allocator
template<typename T, size_t PoolSize = 1024>
class PoolAllocator {
private:
    struct Block {
        alignas(T) char data[sizeof(T)];
        Block* next;
    };

    Block pool_[PoolSize];
    Block* free_list_;
    size_t allocated_count_;

    void initialize() {
        // Link all blocks in free list
        for (size_t i = 0; i < PoolSize - 1; ++i) {
            pool_[i].next = &pool_[i + 1];
        }
        pool_[PoolSize - 1].next = nullptr;
        free_list_ = pool_;
        allocated_count_ = 0;
    }

public:
    using value_type = T;

    PoolAllocator() { initialize(); }

    template<typename U>
    PoolAllocator(const PoolAllocator<U, PoolSize>&) { initialize(); }

    T* allocate(size_t n) {
        if (n != 1) {
            throw std::bad_alloc();  // Pool allocator only handles single objects
        }

        if (!free_list_) {
            throw std::bad_alloc();  // Pool exhausted
        }

        Block* block = free_list_;
        free_list_ = free_list_->next;
        ++allocated_count_;

        std::cout << "Pool allocated object " << allocated_count_ << "/" << PoolSize << "\n";
        return reinterpret_cast<T*>(block);
    }

    void deallocate(T* ptr, size_t n) {
        if (n != 1 || !ptr) return;

        Block* block = reinterpret_cast<Block*>(ptr);
        block->next = free_list_;
        free_list_ = block;
        --allocated_count_;

        std::cout << "Pool deallocated, " << allocated_count_ << "/" << PoolSize << " in use\n";
    }

    size_t available() const {
        return PoolSize - allocated_count_;
    }

    template<typename U>
    bool operator==(const PoolAllocator<U, PoolSize>&) const { return true; }

    template<typename U>
    bool operator!=(const PoolAllocator<U, PoolSize>&) const { return false; }
};

// Stack allocator (for temporary allocations)
template<size_t Size>
class StackAllocator {
private:
    alignas(std::max_align_t) char buffer_[Size];
    size_t offset_;

public:
    StackAllocator() : offset_(0) {}

    template<typename T>
    T* allocate(size_t count) {
        size_t bytes = count * sizeof(T);
        size_t aligned_bytes = (bytes + alignof(T) - 1) & ~(alignof(T) - 1);

        if (offset_ + aligned_bytes > Size) {
            throw std::bad_alloc();
        }

        T* ptr = reinterpret_cast<T*>(buffer_ + offset_);
        offset_ += aligned_bytes;

        std::cout << "Stack allocated " << bytes << " bytes, "
                  << offset_ << "/" << Size << " used\n";
        return ptr;
    }

    template<typename T>
    void deallocate(T* ptr, size_t count) {
        // Stack allocator typically doesn't deallocate individual objects
        // Objects are freed when the entire allocator is reset
        std::cout << "Stack deallocate called (no-op)\n";
    }

    void reset() {
        offset_ = 0;
        std::cout << "Stack allocator reset\n";
    }

    size_t bytes_used() const { return offset_; }
    size_t bytes_available() const { return Size - offset_; }
};

void allocator_examples() {
    std::cout << "=== Basic Allocator ===\n";
    std::vector<int, BasicAllocator<int>> vec1;
    vec1.reserve(10);
    vec1.push_back(42);

    std::cout << "\n=== Pool Allocator ===\n";
    std::vector<Planet, PoolAllocator<Planet, 5>> planets;

    try {
        for (int i = 0; i < 3; ++i) {
            planets.emplace_back("Planet" + std::to_string(i), i * 100.0);
        }
    } catch (const std::bad_alloc& e) {
        std::cout << "Pool exhausted: " << e.what() << "\n";
    }

    std::cout << "\n=== Stack Allocator ===\n";
    StackAllocator<1024> stack_alloc;

    int* nums = stack_alloc.allocate<int>(10);
    double* values = stack_alloc.allocate<double>(5);

    std::cout << "Stack usage: " << stack_alloc.bytes_used() << " bytes\n";

    stack_alloc.deallocate(nums, 10);
    stack_alloc.deallocate(values, 5);

    stack_alloc.reset();
}
```

#### **Memory Pool Implementation**

```cpp
class MemoryPool {
private:
    struct Block {
        size_t size;
        bool is_free;
        Block* next;
        Block* prev;
    };

    char* pool_memory_;
    size_t pool_size_;
    Block* free_blocks_;
    std::mutex pool_mutex_;  // Thread safety

    Block* get_block_header(void* ptr) {
        return reinterpret_cast<Block*>(static_cast<char*>(ptr) - sizeof(Block));
    }

    void* get_block_data(Block* block) {
        return static_cast<char*>(static_cast<void*>(block)) + sizeof(Block);
    }

    void coalesce_free_blocks() {
        Block* current = free_blocks_;

        while (current && current->next) {
            char* current_end = static_cast<char*>(get_block_data(current)) + current->size;
            char* next_start = static_cast<char*>(static_cast<void*>(current->next));

            if (current_end == next_start && current->next->is_free) {
                // Merge blocks
                current->size += sizeof(Block) + current->next->size;
                Block* to_remove = current->next;
                current->next = to_remove->next;

                if (current->next) {
                    current->next->prev = current;
                }
            } else {
                current = current->next;
            }
        }
    }

public:
    MemoryPool(size_t size) : pool_size_(size) {
        pool_memory_ = static_cast<char*>(std::aligned_alloc(alignof(std::max_align_t), size));
        if (!pool_memory_) {
            throw std::bad_alloc();
        }

        // Initialize with one large free block
        free_blocks_ = reinterpret_cast<Block*>(pool_memory_);
        free_blocks_->size = size - sizeof(Block);
        free_blocks_->is_free = true;
        free_blocks_->next = nullptr;
        free_blocks_->prev = nullptr;

        std::cout << "Memory pool created with " << size << " bytes\n";
    }

    ~MemoryPool() {
        std::free(pool_memory_);
        std::cout << "Memory pool destroyed\n";
    }

    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
        std::lock_guard<std::mutex> lock(pool_mutex_);

        // Align size
        size_t aligned_size = (size + alignment - 1) & ~(alignment - 1);

        Block* current = free_blocks_;
        while (current) {
            if (current->is_free && current->size >= aligned_size) {
                // Found suitable block
                current->is_free = false;

                // Split block if it's much larger
                if (current->size > aligned_size + sizeof(Block) + 32) {
                    Block* new_block = reinterpret_cast<Block*>(
                        static_cast<char*>(get_block_data(current)) + aligned_size
                    );

                    new_block->size = current->size - aligned_size - sizeof(Block);
                    new_block->is_free = true;
                    new_block->next = current->next;
                    new_block->prev = current;

                    if (current->next) {
                        current->next->prev = new_block;
                    }
                    current->next = new_block;
                    current->size = aligned_size;
                }

                std::cout << "Allocated " << aligned_size << " bytes from pool\n";
                return get_block_data(current);
            }
            current = current->next;
        }

        throw std::bad_alloc();  // No suitable block found
    }

    void deallocate(void* ptr) {
        if (!ptr) return;

        std::lock_guard<std::mutex> lock(pool_mutex_);

        Block* block = get_block_header(ptr);
        block->is_free = true;

        std::cout << "Deallocated " << block->size << " bytes\n";

        // Coalesce adjacent free blocks
        coalesce_free_blocks();
    }

    void print_stats() {
        std::lock_guard<std::mutex> lock(pool_mutex_);

        size_t free_bytes = 0;
        size_t used_bytes = 0;
        size_t free_blocks = 0;
        size_t used_blocks = 0;

        Block* current = reinterpret_cast<Block*>(pool_memory_);
        char* pool_end = pool_memory_ + pool_size_;

        while (static_cast<char*>(static_cast<void*>(current)) < pool_end) {
            if (current->is_free) {
                free_bytes += current->size;
                ++free_blocks;
            } else {
                used_bytes += current->size;
                ++used_blocks;
            }

            char* next_addr = static_cast<char*>(get_block_data(current)) + current->size;
            if (next_addr >= pool_end) break;

            current = reinterpret_cast<Block*>(next_addr);
        }

        std::cout << "Pool Statistics:\n";
        std::cout << "  Total size: " << pool_size_ << " bytes\n";
        std::cout << "  Used: " << used_bytes << " bytes (" << used_blocks << " blocks)\n";
        std::cout << "  Free: " << free_bytes << " bytes (" << free_blocks << " blocks)\n";
        std::cout << "  Overhead: " << (pool_size_ - used_bytes - free_bytes) << " bytes\n";
    }
};

void memory_pool_example() {
    MemoryPool pool(4096);  // 4KB pool

    // Allocate various sizes
    void* ptr1 = pool.allocate(100);
    void* ptr2 = pool.allocate(200);
    void* ptr3 = pool.allocate(50);

    pool.print_stats();

    // Deallocate some memory
    pool.deallocate(ptr2);

    pool.print_stats();

    // Allocate again (should reuse freed space)
    void* ptr4 = pool.allocate(150);

    pool.print_stats();

    // Clean up
    pool.deallocate(ptr1);
    pool.deallocate(ptr3);
    pool.deallocate(ptr4);

    pool.print_stats();
}
```

### 📊 Memory Debugging and Profiling

#### **Memory Leak Detection**

```cpp
#ifdef DEBUG_MEMORY

class MemoryTracker {
private:
    struct AllocationInfo {
        size_t size;
        const char* file;
        int line;
        std::chrono::time_point<std::chrono::steady_clock> timestamp;
    };

    std::unordered_map<void*, AllocationInfo> allocations_;
    std::mutex tracker_mutex_;
    size_t total_allocated_ = 0;
    size_t peak_allocated_ = 0;
    size_t allocation_count_ = 0;

public:
    static MemoryTracker& instance() {
        static MemoryTracker tracker;
        return tracker;
    }

    void record_allocation(void* ptr, size_t size, const char* file, int line) {
        std::lock_guard<std::mutex> lock(tracker_mutex_);

        allocations_[ptr] = {
            size, file, line, std::chrono::steady_clock::now()
        };

        total_allocated_ += size;
        ++allocation_count_;

        if (total_allocated_ > peak_allocated_) {
            peak_allocated_ = total_allocated_;
        }
    }

    void record_deallocation(void* ptr) {
        std::lock_guard<std::mutex> lock(tracker_mutex_);

        auto it = allocations_.find(ptr);
        if (it != allocations_.end()) {
            total_allocated_ -= it->second.size;
            allocations_.erase(it);
        }
    }

    void report_leaks() {
        std::lock_guard<std::mutex> lock(tracker_mutex_);

        std::cout << "\n=== Memory Leak Report ===\n";
        std::cout << "Current allocations: " << allocations_.size() << "\n";
        std::cout << "Total allocated: " << total_allocated_ << " bytes\n";
        std::cout << "Peak allocated: " << peak_allocated_ << " bytes\n";
        std::cout << "Total allocation count: " << allocation_count_ << "\n";

        if (!allocations_.empty()) {
            std::cout << "\nLeak details:\n";
            for (const auto& [ptr, info] : allocations_) {
                auto duration = std::chrono::steady_clock::now() - info.timestamp;
                auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();

                std::cout << "  " << ptr << ": " << info.size << " bytes, "
                          << info.file << ":" << info.line
                          << " (age: " << seconds << "s)\n";
            }
        } else {
            std::cout << "No memory leaks detected!\n";
        }
        std::cout << "========================\n\n";
    }
};

// Overload global new/delete for tracking
void* operator new(size_t size, const char* file, int line) {
    void* ptr = std::malloc(size);
    if (ptr) {
        MemoryTracker::instance().record_allocation(ptr, size, file, line);
    }
    return ptr;
}

void* operator new[](size_t size, const char* file, int line) {
    void* ptr = std::malloc(size);
    if (ptr) {
        MemoryTracker::instance().record_allocation(ptr, size, file, line);
    }
    return ptr;
}

void operator delete(void* ptr) noexcept {
    if (ptr) {
        MemoryTracker::instance().record_deallocation(ptr);
        std::free(ptr);
    }
}

void operator delete[](void* ptr) noexcept {
    if (ptr) {
        MemoryTracker::instance().record_deallocation(ptr);
        std::free(ptr);
    }
}

// Macros for convenient usage
#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW

#endif  // DEBUG_MEMORY

void memory_debugging_example() {
#ifdef DEBUG_MEMORY
    std::cout << "=== Memory Debugging Example ===\n";

    // These allocations will be tracked
    int* leaked_int = new int(42);
    double* array = new double[100];

    // This will be properly deallocated
    int* proper_int = new int(99);
    delete proper_int;

    // Simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Intentional leak for demonstration
    char* leaked_buffer = new char[1024];

    delete[] array;  // This one is properly cleaned up

    // Report leaks (leaked_int and leaked_buffer should be reported)
    MemoryTracker::instance().report_leaks();

    // Clean up for demo
    delete leaked_int;
    delete[] leaked_buffer;

    MemoryTracker::instance().report_leaks();
#else
    std::cout << "Memory debugging not enabled (DEBUG_MEMORY not defined)\n";
#endif
}
```

### 🎯 Best Practices Summary

#### **Memory Management Guidelines**

```cpp
namespace best_practices {

    // 1. Prefer automatic storage when possible
    void prefer_automatic() {
        // Good: Stack allocation
        std::vector<int> numbers{1, 2, 3, 4, 5};
        std::string text = "Hello World";

        // Good: Smart pointers for heap allocation
        auto planet = std::make_unique<Planet>("Earth", 1000.0);

        // Bad: Manual memory management
        // int* array = new int[100];  // Don't do this!
        // delete[] array;
    }

    // 2. Use RAII consistently
    class GoodResourceManager {
    private:
        std::unique_ptr<ExpensiveResource> resource_;
        std::vector<std::unique_ptr<Component>> components_;

    public:
        GoodResourceManager()
            : resource_(std::make_unique<ExpensiveResource>()) {}

        void addComponent(std::unique_ptr<Component> comp) {
            components_.push_back(std::move(comp));
        }

        // Rule of Zero - no need for custom destructor
        // Smart pointers handle cleanup automatically
    };

    // 3. Avoid raw pointers for ownership
    class BadExample {
        Planet* planets_[10];  // Who owns these? When are they deleted?
    public:
        // Problematic - unclear ownership
    };

    class GoodExample {
        std::vector<std::unique_ptr<Planet>> owned_planets_;      // Clear ownership
        std::vector<std::weak_ptr<Planet>> observed_planets_;     // Clear non-ownership
    public:
        void addPlanet(std::unique_ptr<Planet> planet) {
            owned_planets_.push_back(std::move(planet));
        }

        void observePlanet(std::shared_ptr<Planet> planet) {
            observed_planets_.push_back(planet);
        }
    };

    // 4. Move semantics for performance
    std::vector<std::unique_ptr<Planet>> createPlanets() {
        std::vector<std::unique_ptr<Planet>> planets;

        // Use emplace_back for in-place construction
        planets.emplace_back(std::make_unique<Planet>("Earth", 1000.0));
        planets.emplace_back(std::make_unique<Planet>("Mars", 500.0));

        return planets;  // Move semantics avoid copying
    }

    void usePlanets() {
        auto planets = createPlanets();  // Move, no copy

        // Pass by move when transferring ownership
        auto first_planet = std::move(planets[0]);

        // planets[0] is now nullptr
    }

    // 5. Const correctness for safety
    class ConstCorrectExample {
    private:
        std::vector<std::shared_ptr<const Planet>> readonly_planets_;
        mutable std::mutex cache_mutex_;
        mutable std::optional<double> cached_total_resources_;

    public:
        void addPlanet(std::shared_ptr<const Planet> planet) {
            readonly_planets_.push_back(planet);

            // Invalidate cache
            std::lock_guard<std::mutex> lock(cache_mutex_);
            cached_total_resources_.reset();
        }

        double getTotalResources() const {
            std::lock_guard<std::mutex> lock(cache_mutex_);

            if (!cached_total_resources_) {
                double total = 0.0;
                for (const auto& planet : readonly_planets_) {
                    total += planet->getResources();
                }
                cached_total_resources_ = total;
            }

            return *cached_total_resources_;
        }

        size_t getPlanetCount() const {
            return readonly_planets_.size();
        }
    };

    // 6. Exception safety
    class ExceptionSafeClass {
    private:
        std::vector<std::unique_ptr<Planet>> planets_;
        std::unique_ptr<ExpensiveResource> resource_;

    public:
        void addPlanetSafely(const std::string& name, double resources) {
            // Create new planet first
            auto new_planet = std::make_unique<Planet>(name, resources);

            // If constructor throws, no cleanup needed
            // If this line is reached, we're committed to adding the planet
            planets_.push_back(std::move(new_planet));
        }

        void updateResource() {
            // Create new resource before replacing old one
            auto new_resource = std::make_unique<ExpensiveResource>();

            // Atomic replacement - old resource automatically destroyed
            resource_ = std::move(new_resource);
        }

        // Strong exception guarantee - either succeeds completely or leaves object unchanged
        void replaceAllPlanets(std::vector<std::unique_ptr<Planet>> new_planets) {
            // Use copy-and-swap idiom
            std::vector<std::unique_ptr<Planet>> temp_planets;

            // Copy all planets (may throw)
            for (const auto& planet : new_planets) {
                temp_planets.push_back(
                    std::make_unique<Planet>(planet->getName(), planet->getResources())
                );
            }

            // If we reach here, no exceptions occurred
            // Commit the change (no-throw operations only)
            planets_ = std::move(temp_planets);
        }
    };
}

// 7. Memory-efficient container usage
void container_best_practices() {
    // Reserve space to avoid reallocations
    std::vector<Planet> planets;
    planets.reserve(1000);  // If you know approximate size

    // Use emplace for in-place construction
    planets.emplace_back("Earth", 1000.0);  // Better than push_back

    // Prefer deque for frequent front insertions
    std::deque<Mission> mission_queue;

    // Use appropriate container for access pattern
    std::unordered_map<std::string, Planet> planet_lookup;  // O(1) lookup
    std::map<std::string, Planet> sorted_planets;           // O(log n) but sorted
}

// 8. Custom allocator usage
void allocator_best_practices() {
    // Use pool allocator for frequent same-size allocations
    using PlanetVector = std::vector<Planet, PoolAllocator<Planet, 100>>;
    PlanetVector planets;  // Uses pool allocator

    // Stack allocator for temporary data
    StackAllocator<4096> temp_allocator;

    // RAII wrapper ensures cleanup
    struct TempAllocatorGuard {
        StackAllocator<4096>& allocator;

        TempAllocatorGuard(StackAllocator<4096>& alloc) : allocator(alloc) {}
        ~TempAllocatorGuard() { allocator.reset(); }
    };

    {
        TempAllocatorGuard guard(temp_allocator);

        // Use temp_allocator for temporary data
        auto* temp_data = temp_allocator.allocate<int>(100);

        // Process data...

        // Automatic cleanup when guard goes out of scope
    }
}
```

### 🚨 Common Memory Issues and Solutions

#### **Memory Leaks**

```cpp
namespace memory_issues {

    // BAD: Memory leak
    class LeakyClass {
    private:
        int* data_;

    public:
        LeakyClass() : data_(new int[100]) {}

        // Missing destructor - memory leak!
        // Missing copy constructor - double delete or shallow copy!
        // Missing assignment operator - memory leak!
    };

    // GOOD: RAII approach
    class SafeClass {
    private:
        std::unique_ptr<int[]> data_;

    public:
        SafeClass() : data_(std::make_unique<int[]>(100)) {}

        // Rule of Zero - compiler generates correct copy/move operations
        // Or explicitly delete them if copying doesn't make sense:
        SafeClass(const SafeClass&) = delete;
        SafeClass& operator=(const SafeClass&) = delete;

        // Move operations are fine
        SafeClass(SafeClass&&) = default;
        SafeClass& operator=(SafeClass&&) = default;
    };

    // GOOD: Rule of Five implementation
    class ManualResourceClass {
    private:
        size_t size_;
        int* data_;

    public:
        // Constructor
        ManualResourceClass(size_t size) : size_(size), data_(new int[size]) {}

        // Destructor
        ~ManualResourceClass() {
            delete[] data_;
        }

        // Copy constructor
        ManualResourceClass(const ManualResourceClass& other)
            : size_(other.size_), data_(new int[size_]) {
            std::copy(other.data_, other.data_ + size_, data_);
        }

        // Copy assignment
        ManualResourceClass& operator=(const ManualResourceClass& other) {
            if (this != &other) {
                // Create new resource
                int* new_data = new int[other.size_];
                std::copy(other.data_, other.data_ + other.size_, new_data);

                // Clean up old resource
                delete[] data_;

                // Update state
                data_ = new_data;
                size_ = other.size_;
            }
            return *this;
        }

        // Move constructor
        ManualResourceClass(ManualResourceClass&& other) noexcept
            : size_(other.size_), data_(other.data_) {
            other.size_ = 0;
            other.data_ = nullptr;
        }

        // Move assignment
        ManualResourceClass& operator=(ManualResourceClass&& other) noexcept {
            if (this != &other) {
                delete[] data_;  // Clean up old resource

                size_ = other.size_;
                data_ = other.data_;

                other.size_ = 0;
                other.data_ = nullptr;
            }
            return *this;
        }

        size_t size() const { return size_; }
        int& operator[](size_t index) { return data_[index]; }
        const int& operator[](size_t index) const { return data_[index]; }
    };
}
```

#### **Dangling Pointers and Use-After-Free**

```cpp
namespace dangling_pointers {

    // BAD: Dangling pointer
    int* bad_function() {
        int local_var = 42;
        return &local_var;  // Returns address of local variable - UNDEFINED BEHAVIOR!
    }

    // BAD: Use after free
    void bad_usage() {
        int* ptr = new int(42);
        delete ptr;

        std::cout << *ptr << "\n";  // Use after free - UNDEFINED BEHAVIOR!
        delete ptr;  // Double delete - UNDEFINED BEHAVIOR!
    }

    // GOOD: Safe alternatives
    std::unique_ptr<int> safe_function() {
        return std::make_unique<int>(42);  // Returns owned resource
    }

    void safe_usage() {
        auto ptr = std::make_unique<int>(42);

        std::cout << *ptr << "\n";  // Safe access

        // Automatic cleanup, no double-delete possible
    }

    // GOOD: Using weak_ptr to avoid dangling references
    class SafeObserver {
    private:
        std::weak_ptr<Planet> observed_planet_;

    public:
        void observe(std::shared_ptr<Planet> planet) {
            observed_planet_ = planet;  // weak_ptr doesn't affect lifetime
        }

        void checkPlanet() {
            if (auto planet = observed_planet_.lock()) {  // Safe access
                std::cout << "Planet " << planet->getName() << " is alive\n";
            } else {
                std::cout << "Planet has been destroyed\n";
            }
        }
    };
}
```

#### **Buffer Overruns and Bounds Checking**

```cpp
namespace buffer_safety {

    // BAD: Buffer overrun
    void unsafe_buffer() {
        char buffer[10];
        strcpy(buffer, "This string is way too long!");  // Buffer overrun!

        int array[5];
        array[10] = 42;  // Out of bounds access!
    }

    // GOOD: Safe alternatives
    void safe_buffer() {
        // Use std::string instead of char arrays
        std::string safe_string = "This string can be any length!";

        // Use std::vector with bounds checking
        std::vector<int> safe_array(5);
        safe_array.at(4) = 42;  // Bounds checked access

        try {
            safe_array.at(10) = 42;  // Throws std::out_of_range
        } catch (const std::out_of_range& e) {
            std::cout << "Bounds check caught: " << e.what() << "\n";
        }

        // Use std::array for fixed-size arrays
        std::array<int, 5> fixed_array;
        fixed_array.fill(0);
        fixed_array[4] = 42;  // No bounds check in release mode
        fixed_array.at(4) = 42;  // Bounds checked
    }

    // Safe string operations
    void safe_string_ops() {
        std::string source = "Hello, World!";

        // Safe copying
        std::string destination = source;  // Automatic memory management

        // Safe concatenation
        std::string result = source + " Additional text";

        // Safe substring
        std::string sub = source.substr(0, 5);  // "Hello"

        // Safe search
        size_t pos = source.find("World");
        if (pos != std::string::npos) {
            std::cout << "Found at position " << pos << "\n";
        }
    }
}
```

### 📊 Performance Considerations

#### **Memory Access Patterns**

```cpp
namespace performance {

    // Cache-friendly data structures
    struct SoA_Particles {  // Structure of Arrays
        std::vector<float> x_positions;
        std::vector<float> y_positions;
        std::vector<float> z_positions;
        std::vector<float> velocities;

        void update_positions(size_t count) {
            // Cache-friendly - processes same data type together
            for (size_t i = 0; i < count; ++i) {
                x_positions[i] += velocities[i];
                y_positions[i] += velocities[i];
                z_positions[i] += velocities[i];
            }
        }
    };

    struct AoS_Particle {  // Array of Structures
        float x, y, z;
        float velocity;
        // Other data members...
        char padding[64];  // May cause cache misses
    };

    class AoS_Particles {
        std::vector<AoS_Particle> particles;

    public:
        void update_positions() {
            // Less cache-friendly - loads entire particle structure
            for (auto& particle : particles) {
                particle.x += particle.velocity;
                particle.y += particle.velocity;
                particle.z += particle.velocity;
            }
        }
    };

    // Memory prefetching for performance
    void prefetch_example(const std::vector<Planet*>& planets) {
        for (size_t i = 0; i < planets.size(); ++i) {
            // Prefetch next planet while processing current
            if (i + 1 < planets.size()) {
                __builtin_prefetch(planets[i + 1], 0, 3);  // GCC/Clang specific
            }

            // Process current planet
            planets[i]->update();
        }
    }

    // Object pooling for frequent allocations
    template<typename T>
    class ObjectPool {
    private:
        std::stack<std::unique_ptr<T>> available_objects_;
        std::mutex pool_mutex_;

    public:
        template<typename... Args>
        std::unique_ptr<T> acquire(Args&&... args) {
            std::lock_guard<std::mutex> lock(pool_mutex_);

            if (!available_objects_.empty()) {
                auto obj = std::move(available_objects_.top());
                available_objects_.pop();

                // Reinitialize object
                *obj = T(std::forward<Args>(args)...);
                return obj;
            }

            // Create new object if pool is empty
            return std::make_unique<T>(std::forward<Args>(args)...);
        }

        void release(std::unique_ptr<T> obj) {
            if (obj) {
                std::lock_guard<std::mutex> lock(pool_mutex_);
                available_objects_.push(std::move(obj));
            }
        }

        size_t available_count() const {
            std::lock_guard<std::mutex> lock(pool_mutex_);
            return available_objects_.size();
        }
    };

    void object_pool_example() {
        ObjectPool<Planet> planet_pool;

        // Acquire objects from pool
        auto earth = planet_pool.acquire("Earth", 1000.0);
        auto mars = planet_pool.acquire("Mars", 500.0);

        // Use objects...
        earth->addResources(100.0);

        // Return to pool for reuse
        planet_pool.release(std::move(earth));
        planet_pool.release(std::move(mars));

        std::cout << "Available in pool: " << planet_pool.available_count() << "\n";
    }
}
```

### 📋 Memory Management Quick Reference

| Scenario                 | Recommended Approach           | Example                                           |
| ------------------------ | ------------------------------ | ------------------------------------------------- |
| **Single ownership**     | `std::unique_ptr`              | `auto planet = std::make_unique<Planet>()`        |
| **Shared ownership**     | `std::shared_ptr`              | `auto shared_planet = std::make_shared<Planet>()` |
| **Non-owning reference** | `std::weak_ptr` or raw pointer | `std::weak_ptr<Planet> observer`                  |
| **Fixed-size array**     | `std::array`                   | `std::array<int, 10> numbers`                     |
| **Dynamic array**        | `std::vector`                  | `std::vector<Planet> planets`                     |
| **String data**          | `std::string`                  | `std::string name = "Earth"`                      |
| **Temporary data**       | Stack allocation               | `Planet local_planet("Mars", 500.0)`              |
| **Large objects**        | Heap with smart pointers       | `auto large_obj = std::make_unique<LargeClass>()` |
| **C API integration**    | RAII wrapper                   | Custom wrapper with destructor                    |
| **Custom allocation**    | Custom allocator               | Pool or stack allocator                           |

### 🎯 Key Takeaways

#### **Golden Rules of Memory Management**

1. **RAII Everything** - Tie resource lifetime to object lifetime
2. **Smart Pointers First** - Avoid raw pointers for ownership
3. **Const Correctness** - Use const to prevent accidental modifications
4. **Move Semantics** - Avoid unnecessary copies of expensive objects
5. **Exception Safety** - Ensure resources are cleaned up even with exceptions
6. **No Naked New/Delete** - Use smart pointers and containers instead
7. **Clear Ownership** - Make ownership semantics explicit in your design
8. **Test for Leaks** - Use tools like Valgrind, AddressSanitizer, or custom tracking
9. **Profile Memory Usage** - Measure and optimize memory access patterns
10. **Consider Cache Locality** - Organize data for efficient memory access

#### **Memory Safety Checklist**

- ✅ Use smart pointers for ownership
- ✅ Implement Rule of Zero, Three, or Five correctly
- ✅ Check for null pointers before dereferencing
- ✅ Use bounds-checked access when needed
- ✅ Avoid returning references to local variables
- ✅ Initialize all member variables
- ✅ Use RAII for all resources
- ✅ Prefer containers over raw arrays
- ✅ Handle exceptions properly
- ✅ Test with memory debugging tools

#### **Performance Optimization Tips**

- 🚀 Reserve container capacity when size is known
- 🚀 Use move semantics to avoid copying
- 🚀 Consider object pooling for frequent allocations
- 🚀 Choose appropriate data structures (SoA vs AoS)
- 🚀 Minimize memory fragmentation
- 🚀 Use custom allocators for specialized needs
- 🚀 Profile memory access patterns
- 🚀 Consider cache locality in data layout
- 🚀 Avoid unnecessary dynamic allocations
- 🚀 Use stack allocation when possible

---

_This cheat sheet provides comprehensive coverage of modern C++ memory management. For practical implementations, see the `src/memory/` directory in CppVerseHub._
