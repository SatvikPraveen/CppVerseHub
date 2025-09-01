/**
 * @file CoroutinesDemo.hpp
 * @brief Comprehensive C++20 coroutines showcase and demonstrations
 * @details File location: src/concurrency/CoroutinesDemo.hpp
 * 
 * This file demonstrates C++20 coroutines including generators, tasks,
 * async operations, and advanced coroutine patterns for concurrent programming.
 * Requires C++20 compiler support with coroutines.
 */

#ifndef COROUTINESDEMO_HPP
#define COROUTINESDEMO_HPP

#include <coroutine>
#include <thread>
#include <vector>
#include <queue>
#include <memory>
#include <chrono>
#include <iostream>
#include <string>
#include <random>
#include <functional>
#include <optional>
#include <variant>
#include <future>
#include <atomic>
#include <exception>

namespace CppVerseHub::Concurrency {

    /**
     * @class Generator
     * @brief Basic generator implementation using coroutines
     */
    template<typename T>
    class Generator {
    public:
        struct promise_type {
            T current_value;
            
            Generator get_return_object() {
                return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
            }
            
            std::suspend_always initial_suspend() { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }
            
            std::suspend_always yield_value(T value) {
                current_value = value;
                return {};
            }
            
            void return_void() {}
            void unhandled_exception() { std::terminate(); }
        };

        using handle_type = std::coroutine_handle<promise_type>;

        Generator(handle_type h) : coro_handle_(h) {}
        
        ~Generator() {
            if (coro_handle_) {
                coro_handle_.destroy();
            }
        }
        
        // Move only
        Generator(const Generator&) = delete;
        Generator& operator=(const Generator&) = delete;
        
        Generator(Generator&& other) noexcept : coro_handle_(other.coro_handle_) {
            other.coro_handle_ = {};
        }
        
        Generator& operator=(Generator&& other) noexcept {
            if (this != &other) {
                if (coro_handle_) {
                    coro_handle_.destroy();
                }
                coro_handle_ = other.coro_handle_;
                other.coro_handle_ = {};
            }
            return *this;
        }

        bool next() {
            if (coro_handle_ && !coro_handle_.done()) {
                coro_handle_.resume();
                return !coro_handle_.done();
            }
            return false;
        }

        T value() const {
            return coro_handle_.promise().current_value;
        }

        bool done() const {
            return coro_handle_.done();
        }

        // Iterator support
        class iterator {
        public:
            iterator(Generator& gen, bool is_end = false) : gen_(gen), is_end_(is_end) {
                if (!is_end_ && gen_.coro_handle_ && !gen_.coro_handle_.done()) {
                    gen_.coro_handle_.resume();
                    is_end_ = gen_.coro_handle_.done();
                }
            }

            iterator& operator++() {
                if (gen_.coro_handle_ && !gen_.coro_handle_.done()) {
                    gen_.coro_handle_.resume();
                    is_end_ = gen_.coro_handle_.done();
                }
                return *this;
            }

            T operator*() const {
                return gen_.coro_handle_.promise().current_value;
            }

            bool operator==(const iterator& other) const {
                return is_end_ == other.is_end_;
            }

            bool operator!=(const iterator& other) const {
                return !(*this == other);
            }

        private:
            Generator& gen_;
            bool is_end_;
        };

        iterator begin() { return iterator(*this); }
        iterator end() { return iterator(*this, true); }

    private:
        handle_type coro_handle_;
    };

    /**
     * @class Task
     * @brief Async task implementation using coroutines
     */
    template<typename T>
    class Task {
    public:
        struct promise_type {
            std::variant<std::monostate, T, std::exception_ptr> result_;

            Task get_return_object() {
                return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            std::suspend_never initial_suspend() { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }

            template<typename U>
            void return_value(U&& value) {
                result_ = std::forward<U>(value);
            }

            void unhandled_exception() {
                result_ = std::current_exception();
            }
        };

        using handle_type = std::coroutine_handle<promise_type>;

        Task(handle_type h) : coro_handle_(h) {}

        ~Task() {
            if (coro_handle_) {
                coro_handle_.destroy();
            }
        }

        // Move only
        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;

        Task(Task&& other) noexcept : coro_handle_(other.coro_handle_) {
            other.coro_handle_ = {};
        }

        Task& operator=(Task&& other) noexcept {
            if (this != &other) {
                if (coro_handle_) {
                    coro_handle_.destroy();
                }
                coro_handle_ = other.coro_handle_;
                other.coro_handle_ = {};
            }
            return *this;
        }

        bool is_ready() const {
            return coro_handle_ && coro_handle_.done();
        }

        T get() {
            if (!coro_handle_) {
                throw std::runtime_error("Task has no coroutine handle");
            }

            if (!coro_handle_.done()) {
                throw std::runtime_error("Task is not completed");
            }

            auto& result = coro_handle_.promise().result_;
            
            if (std::holds_alternative<std::exception_ptr>(result)) {
                std::rethrow_exception(std::get<std::exception_ptr>(result));
            }
            
            if (std::holds_alternative<T>(result)) {
                return std::get<T>(result);
            }
            
            throw std::runtime_error("Task has no result");
        }

        // Awaitable interface
        bool await_ready() const { return is_ready(); }
        
        void await_suspend(std::coroutine_handle<> continuation) {
            // In a full implementation, this would schedule the continuation
            // For this demo, we'll just resume immediately
            if (coro_handle_ && !coro_handle_.done()) {
                coro_handle_.resume();
            }
            continuation.resume();
        }
        
        T await_resume() { return get(); }

    private:
        handle_type coro_handle_;
    };

    /**
     * @class AsyncGenerator
     * @brief Asynchronous generator using coroutines
     */
    template<typename T>
    class AsyncGenerator {
    public:
        struct promise_type {
            T current_value;
            std::coroutine_handle<> continuation_;

            AsyncGenerator get_return_object() {
                return AsyncGenerator{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            std::suspend_always initial_suspend() { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }

            std::suspend_always yield_value(T value) {
                current_value = value;
                return {};
            }

            void return_void() {}
            void unhandled_exception() { std::terminate(); }
        };

        using handle_type = std::coroutine_handle<promise_type>;

        AsyncGenerator(handle_type h) : coro_handle_(h) {}

        ~AsyncGenerator() {
            if (coro_handle_) {
                coro_handle_.destroy();
            }
        }

        // Move only
        AsyncGenerator(const AsyncGenerator&) = delete;
        AsyncGenerator& operator=(const AsyncGenerator&) = delete;

        AsyncGenerator(AsyncGenerator&& other) noexcept : coro_handle_(other.coro_handle_) {
            other.coro_handle_ = {};
        }

        AsyncGenerator& operator=(AsyncGenerator&& other) noexcept {
            if (this != &other) {
                if (coro_handle_) {
                    coro_handle_.destroy();
                }
                coro_handle_ = other.coro_handle_;
                other.coro_handle_ = {};
            }
            return *this;
        }

        // Async iterator
        class async_iterator {
        public:
            async_iterator(AsyncGenerator& gen, bool is_end = false) : gen_(gen), is_end_(is_end) {}

            Task<bool> next() {
                if (gen_.coro_handle_ && !gen_.coro_handle_.done()) {
                    gen_.coro_handle_.resume();
                    co_return !gen_.coro_handle_.done();
                }
                co_return false;
            }

            T value() const {
                return gen_.coro_handle_.promise().current_value;
            }

        private:
            AsyncGenerator& gen_;
            bool is_end_;
        };

        async_iterator begin() { return async_iterator(*this); }
        async_iterator end() { return async_iterator(*this, true); }

    private:
        handle_type coro_handle_;
    };

    /**
     * @class CoroutineScheduler
     * @brief Simple scheduler for coroutines
     */
    class CoroutineScheduler {
    public:
        CoroutineScheduler(size_t num_threads = std::thread::hardware_concurrency());
        ~CoroutineScheduler();

        void schedule(std::coroutine_handle<> coro);
        void start();
        void stop();
        
        size_t pending_tasks() const;
        bool is_running() const { return running_; }

        // Awaitable for yielding execution
        struct yield_awaitable {
            CoroutineScheduler* scheduler_;

            bool await_ready() const noexcept { return false; }
            
            void await_suspend(std::coroutine_handle<> coro) const {
                scheduler_->schedule(coro);
            }
            
            void await_resume() const noexcept {}
        };

        yield_awaitable yield() { return yield_awaitable{this}; }

    private:
        std::vector<std::thread> workers_;
        std::queue<std::coroutine_handle<>> task_queue_;
        std::mutex queue_mutex_;
        std::condition_variable queue_condition_;
        std::atomic<bool> running_{false};

        void worker_thread();
    };

    /**
     * @class AsyncFileReader
     * @brief Simulated async file reader using coroutines
     */
    class AsyncFileReader {
    public:
        struct FileData {
            std::string filename;
            std::string content;
            bool success;
            std::string error_message;
        };

        static Task<FileData> read_file_async(const std::string& filename);
        static Task<std::vector<FileData>> read_multiple_files(const std::vector<std::string>& filenames);

    private:
        static FileData simulate_file_read(const std::string& filename);
    };

    /**
     * @class NetworkClient
     * @brief Simulated async network client using coroutines
     */
    class NetworkClient {
    public:
        struct Response {
            int status_code;
            std::string body;
            std::chrono::milliseconds latency;
            bool success;
        };

        Task<Response> get_async(const std::string& url);
        Task<Response> post_async(const std::string& url, const std::string& data);
        Task<std::vector<Response>> batch_requests(const std::vector<std::string>& urls);

    private:
        Response simulate_http_request(const std::string& url, const std::string& method = "GET", const std::string& data = "");
        std::random_device rd_;
        std::mt19937 gen_{rd_()};
    };

    /**
     * @class ProducerConsumerCoroutines
     * @brief Producer-consumer pattern using coroutines
     */
    class ProducerConsumerCoroutines {
    public:
        struct Message {
            int id;
            std::string content;
            std::chrono::steady_clock::time_point timestamp;
        };

        ProducerConsumerCoroutines(size_t buffer_capacity = 10);
        
        Task<void> producer_task(const std::string& producer_name, int message_count);
        Task<void> consumer_task(const std::string& consumer_name, int message_count);
        
        void start_demo(int num_producers = 2, int num_consumers = 3, int messages_per_producer = 10);
        void print_statistics() const;

    private:
        std::queue<Message> buffer_;
        std::mutex buffer_mutex_;
        std::condition_variable not_empty_;
        std::condition_variable not_full_;
        size_t capacity_;
        std::atomic<int> message_id_counter_{0};
        std::atomic<int> total_produced_{0};
        std::atomic<int> total_consumed_{0};

        struct MessageAwaitable {
            ProducerConsumerCoroutines* parent_;
            std::optional<Message> message_;

            bool await_ready() const;
            void await_suspend(std::coroutine_handle<> coro);
            std::optional<Message> await_resume() const { return message_; }
        };

        MessageAwaitable try_consume();
    };

    /**
     * @class WebCrawler
     * @brief Async web crawler simulation using coroutines
     */
    class WebCrawler {
    public:
        struct CrawlResult {
            std::string url;
            std::vector<std::string> found_links;
            size_t word_count;
            std::chrono::milliseconds processing_time;
            bool success;
            std::string error;
        };

        WebCrawler(size_t max_concurrent_requests = 5);

        Task<CrawlResult> crawl_page(const std::string& url);
        Task<std::vector<CrawlResult>> crawl_website(const std::string& base_url, int max_depth = 2);
        
        void start_crawl_demo();

    private:
        size_t max_concurrent_;
        std::atomic<size_t> active_requests_{0};
        NetworkClient network_client_;

        CrawlResult simulate_page_crawl(const std::string& url);
        std::vector<std::string> extract_links(const std::string& url, const std::string& content);
    };

    /**
     * @class DataProcessor
     * @brief Pipeline data processor using coroutines
     */
    class DataProcessor {
    public:
        struct ProcessingStage {
            std::string name;
            std::function<std::string(const std::string&)> transform;
            std::chrono::milliseconds processing_time;
        };

        DataProcessor();
        
        Task<std::string> process_data_async(const std::string& input);
        Generator<std::string> process_stream(const std::vector<std::string>& inputs);
        AsyncGenerator<std::string> process_stream_async(const std::vector<std::string>& inputs);
        
        void demonstrate_pipeline();

    private:
        std::vector<ProcessingStage> pipeline_;
        CoroutineScheduler scheduler_;
    };

    /**
     * @class BasicCoroutinesDemo
     * @brief Basic coroutine demonstrations and examples
     */
    class BasicCoroutinesDemo {
    public:
        static void demonstrate_generators();
        static void demonstrate_tasks();
        static void demonstrate_async_generators();
        static void demonstrate_coroutine_scheduler();

        // Example generator functions
        static Generator<int> fibonacci_generator(int count);
        static Generator<std::string> string_generator(const std::vector<std::string>& strings);
        static AsyncGenerator<int> async_number_generator(int start, int end);

        // Example task functions
        static Task<int> compute_factorial(int n);
        static Task<std::string> async_string_operation(const std::string& input);
        static Task<std::vector<int>> parallel_computation(const std::vector<int>& inputs);

    private:
        static void simulate_async_work(std::chrono::milliseconds duration);
    };

    /**
     * @class CoroutinesDemo
     * @brief Main demonstration coordinator for C++20 coroutines
     */
    class CoroutinesDemo {
    public:
        static void demonstrate_basic_coroutines();
        static void demonstrate_async_file_operations();
        static void demonstrate_network_coroutines();
        static void demonstrate_producer_consumer_coroutines();
        static void demonstrate_web_crawler();
        static void demonstrate_data_processing_pipeline();
        static void run_all_demonstrations();

    private:
        static void print_section_header(const std::string& title);
        static void print_section_footer();
        static void simulate_work(std::chrono::milliseconds duration);
    };

} // namespace CppVerseHub::Concurrency

#endif // COROUTINESDEMO_HPP