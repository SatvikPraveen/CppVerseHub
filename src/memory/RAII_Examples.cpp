/**
 * @file RAII_Examples.cpp
 * @brief FileRAII / NetworkConnection implementation and the RAII showcase.
 */

#include "memory/RAII_Examples.hpp"

#include <system_error>

namespace CppVerseHub::Memory {

    // ------------------------------------------------------------------------ FileRAII

    namespace {
        std::FILE* open_file(const std::filesystem::path& path, const char* mode) noexcept {
#if defined(_MSC_VER)
            std::FILE* f = nullptr;
            const std::string m(mode);
            const std::wstring wmode(m.begin(), m.end());
            return _wfopen_s(&f, path.c_str(), wmode.c_str()) == 0 ? f : nullptr;
#else
            return std::fopen(path.string().c_str(), mode);
#endif
        }
    } // namespace

    FileRAII::FileRAII(const std::filesystem::path& path, const char* mode)
        : file_(open_file(path, mode)), path_(path) {
        if (!file_) {
            throw std::runtime_error("FileRAII: failed to open '" + path.string() + "'");
        }
    }

    bool FileRAII::write(std::string_view data) {
        if (!file_) {
            return false;
        }
        return std::fwrite(data.data(), 1, data.size(), file_.get()) == data.size();
    }

    std::string FileRAII::read_all() {
        if (!file_) {
            throw std::runtime_error("FileRAII: file is closed");
        }
        static_cast<void>(std::fflush(file_.get()));
        std::rewind(file_.get());
        std::string content;
        char buffer[4096];
        std::size_t n = 0;
        while ((n = std::fread(buffer, 1, sizeof(buffer), file_.get())) > 0) {
            content.append(buffer, n);
        }
        if (std::ferror(file_.get()) != 0) {
            throw std::runtime_error("FileRAII: read error on '" + path_.string() + "'");
        }
        return content;
    }

    bool FileRAII::flush() noexcept { return file_ && std::fflush(file_.get()) == 0; }

    bool FileRAII::close() noexcept {
        if (!file_) {
            return true;
        }
        std::FILE* f = file_.release();
        return std::fclose(f) == 0;
    }

    // --------------------------------------------------------------- NetworkConnection

    NetworkConnection::NetworkConnection(std::string address) : address_(std::move(address)) {
        if (address_.find(':') == std::string::npos) {
            throw std::invalid_argument("NetworkConnection: address must be host:port");
        }
        connected_ = true;
        active_.fetch_add(1, std::memory_order_relaxed);
    }

    NetworkConnection::NetworkConnection(NetworkConnection&& other) noexcept
        : address_(std::move(other.address_)),
          bytes_sent_(std::exchange(other.bytes_sent_, 0)),
          connected_(std::exchange(other.connected_, false)) {}

    NetworkConnection& NetworkConnection::operator=(NetworkConnection&& other) noexcept {
        if (this != &other) {
            disconnect();
            address_ = std::move(other.address_);
            bytes_sent_ = std::exchange(other.bytes_sent_, 0);
            connected_ = std::exchange(other.connected_, false);
        }
        return *this;
    }

    NetworkConnection::~NetworkConnection() { disconnect(); }

    std::size_t NetworkConnection::send(std::string_view message) {
        if (!connected_) {
            throw std::logic_error("NetworkConnection: send on a closed connection");
        }
        bytes_sent_ += message.size();
        return message.size();
    }

    void NetworkConnection::disconnect() noexcept {
        if (connected_) {
            connected_ = false;
            active_.fetch_sub(1, std::memory_order_relaxed);
        }
    }

    // ------------------------------------------------------------------------ Showcase

    namespace {
        struct Buffer {
            std::vector<int> data = std::vector<int>(16, 0);
        };
    } // namespace

    void demonstrateRAII(std::ostream& out) {
        out << "=== RAII ===\n";
        // Restore the caller's stream formatting however we leave this function.
        const auto restore_flags = make_scope_guard([&out, flags = out.flags()]() noexcept { out.flags(flags); });

        // 1. Generic handle wrapper.
        {
            MockOsHandle a(MockOsHandleTraits::open());
            MockOsHandle b(MockOsHandleTraits::open());
            out << "UniqueHandle: open handles " << MockOsHandleTraits::open_count() << "\n";
            b = std::move(a); // b's old handle closed, a's moved in
            out << "After move-assign: open handles " << MockOsHandleTraits::open_count() << "\n";
        }
        out << "After scope: open handles " << MockOsHandleTraits::open_count() << "\n";

        // 2. File wrapper (temp directory, cleaned up by a scope guard).
        try {
            const auto path = std::filesystem::temp_directory_path() / "cppversehub_memory_demo.txt";
            const auto remove_file = make_scope_guard([&path]() noexcept {
                std::error_code ec;
                std::filesystem::remove(path, ec);
            });
            FileRAII file(path, "w+");
            static_cast<void>(file.write("RAII closes files even when exceptions fly.\n"));
            out << "FileRAII read back: " << file.read_all();
        } catch (const std::exception& e) {
            out << "FileRAII unavailable: " << e.what() << "\n";
        }

        // 3. Timer.
        std::chrono::nanoseconds elapsed{};
        const long long sum = RAIIUtils::measure(elapsed, [] {
            long long s = 0;
            for (int i = 0; i < 10000; ++i) {
                s += i;
            }
            return s;
        });
        out << "TimerRAII: sum = " << sum << " (timed " << (elapsed.count() >= 0 ? "ok" : "??") << ")\n";

        // 4. Scoped lock with early unlock.
        std::mutex m;
        {
            ScopedLock lock(m);
            out << "ScopedLock owns lock: " << std::boolalpha << lock.owns_lock();
            lock.unlock();
            out << ", after unlock: " << lock.owns_lock() << "\n";
        }

        // 5. Scope guards: commit / rollback.
        std::vector<std::string> journal;
        try {
            const auto always = make_scope_guard([&journal]() noexcept { journal.emplace_back("cleanup"); });
            const auto rollback = make_scope_fail([&journal]() noexcept { journal.emplace_back("rollback"); });
            const auto commit = make_scope_success([&journal]() noexcept { journal.emplace_back("commit"); });
            throw std::runtime_error("transaction failed");
        } catch (const std::exception&) {
            journal.emplace_back("handled");
        }
        out << "Scope guards on failure:";
        for (const auto& entry : journal) {
            out << ' ' << entry;
        }
        out << "\n";

        // 6. Resource pool with leases.
        ResourcePool<Buffer> pool(2);
        {
            auto l1 = pool.acquire();
            auto l2 = pool.acquire();
            auto l3 = pool.acquire(); // grows the pool
            l1->data[0] = 42;
            out << "ResourcePool: leased " << pool.leased_count() << ", idle " << pool.idle_count() << ", created "
                << pool.created_count() << "\n";
        }
        out << "After leases end: leased " << pool.leased_count() << ", idle " << pool.idle_count() << "\n";

        // 7. Connection: failure in constructor acquires nothing.
        {
            NetworkConnection conn("earth.example:4242");
            static_cast<void>(conn.send("hello mars"));
            try {
                const NetworkConnection bad("no-port-here");
            } catch (const std::invalid_argument&) {
                out << "Malformed address rejected; active connections still "
                    << NetworkConnection::active_connections() << "\n";
            }
            NetworkConnection moved(std::move(conn));
            out << "Moved connection sent " << moved.bytes_sent() << " bytes\n";
        }
        out << "Active connections after scope: " << NetworkConnection::active_connections() << "\n";

        // 8. Rule-of-five array with strong guarantee.
        RAIIUtils::ArrayRAII<std::string> names(3, "probe");
        RAIIUtils::ArrayRAII<std::string> copy = names;
        copy[0] = "lander";
        out << "ArrayRAII: original[0] = " << names[0] << ", copy[0] = " << copy[0] << "\n";
    }

} // namespace CppVerseHub::Memory
