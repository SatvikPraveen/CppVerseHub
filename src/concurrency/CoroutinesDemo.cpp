/**
 * @file CoroutinesDemo.cpp
 * @brief Implementation of comprehensive C++20 coroutines demonstrations
 * @details File location: src/concurrency/CoroutinesDemo.cpp
 */

#include "CoroutinesDemo.hpp"
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace CppVerseHub::Concurrency {

    // ========== CoroutineScheduler Implementation ==========

    CoroutineScheduler::CoroutineScheduler(size_t num_threads) {
        workers_.reserve(num_threads);
    }

    CoroutineScheduler::~CoroutineScheduler() {
        stop();
    }

    void CoroutineScheduler::schedule(std::coroutine_handle<> coro) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            task_queue_.push(coro);
        }
        queue_condition_.notify_one();
    }

    void CoroutineScheduler::start() {
        running_ = true;
        
        for (size_t i = 0; i < workers_.capacity(); ++i) {
            workers_.emplace_back(&CoroutineScheduler::worker_thread, this);
        }
    }

    void CoroutineScheduler::stop() {
        running_ = false;
        queue_condition_.notify_all();
        
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        
        workers_.clear();
    }

    size_t CoroutineScheduler::pending_tasks() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return task_queue_.size();
    }

    void CoroutineScheduler::worker_thread() {
        while (running_) {
            std::coroutine_handle<> coro;
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                queue_condition_.wait(lock, [this] { return !running_ || !task_queue_.empty(); });
                
                if (!running_) break;
                
                if (!task_queue_.empty()) {
                    coro = task_queue_.front();
                    task_queue_.pop();
                }
            }
            
            if (coro) {
                coro.resume();
            }
        }
    }

    // ========== AsyncFileReader Implementation ==========

    Task<AsyncFileReader::FileData> AsyncFileReader::read_file_async(const std::string& filename) {
        // Simulate async file reading
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        co_return simulate_file_read(filename);
    }

    Task<std::vector<AsyncFileReader::FileData>> 
    AsyncFileReader::read_multiple_files(const std::vector<std::string>& filenames) {
        std::vector<FileData> results;
        results.reserve(filenames.size());
        
        for (const auto& filename : filenames) {
            auto file_data = co_await read_file_async(filename);
            results.push_back(file_data);
        }
        
        co_return results;
    }

    AsyncFileReader::FileData AsyncFileReader::simulate_file_read(const std::string& filename) {
        // Simulate different file scenarios
        FileData data;
        data.filename = filename;
        
        if (filename.find("error") != std::string::npos) {
            data.success = false;
            data.error_message = "File not found: " + filename;
            data.content = "";
        } else {
            data.success = true;
            data.error_message = "";
            data.content = "Content of " + filename + " - Lorem ipsum dolor sit amet...";
        }
        
        return data;
    }

    // ========== NetworkClient Implementation ==========

    Task<NetworkClient::Response> NetworkClient::get_async(const std::string& url) {
        co_return simulate_http_request(url, "GET");
    }

    Task<NetworkClient::Response> NetworkClient::post_async(const std::string& url, const std::string& data) {
        co_return simulate_http_request(url, "POST", data);
    }

    Task<std::vector<NetworkClient::Response>> 
    NetworkClient::batch_requests(const std::vector<std::string>& urls) {
        std::vector<Response> responses;
        responses.reserve(urls.size());
        
        for (const auto& url : urls) {
            auto response = co_await get_async(url);
            responses.push_back(response);
        }
        
        co_return responses;
    }

    NetworkClient::Response NetworkClient::simulate_http_request(const std::string& url, 
                                                                const std::string& method, 
                                                                const std::string& data) {
        // Simulate network latency
        std::uniform_int_distribution<> latency_dist(50, 500);
        auto latency = std::chrono::milliseconds(latency_dist(gen_));
        std::this_thread::sleep_for(latency);
        
        Response response;
        response.latency = latency;
        
        // Simulate different response scenarios
        std::uniform_int_distribution<> success_dist(1, 100);
        if (success_dist(gen_) <= 85) { // 85% success rate
            response.success = true;
            response.status_code = 200;
            response.body = "Response from " + url + " via " + method;
            if (!data.empty()) {
                response.body += " with data: " + data.substr(0, 50);
            }
        } else {
            response.success = false;
            response.status_code = 500;
            response.body = "Internal Server Error";
        }
        
        return response;
    }

    // ========== ProducerConsumerCoroutines Implementation ==========

    ProducerConsumerCoroutines::ProducerConsumerCoroutines(size_t buffer_capacity) 
        : capacity_(buffer_capacity) {}

    Task<void> ProducerConsumerCoroutines::producer_task(const std::string& producer_name, int message_count) {
        for (int i = 0; i < message_count; ++i) {
            Message msg;
            msg.id = ++message_id_counter_;
            msg.content = "Message from " + producer_name + " #" + std::to_string(i);
            msg.timestamp = std::chrono::steady_clock::now();
            
            // Wait for buffer space
            std::unique_lock<std::mutex> lock(buffer_mutex_);
            co_await [this, &lock]() -> Task<void> {
                not_full_.wait(lock, [this] { return buffer_.size() < capacity_; });
                co_return;
            }();
            
            buffer_.push(msg);
            total_produced_++;
            not_empty_.notify_one();
            
            std::cout << "📦 " << producer_name << " produced: " << msg.content 
                      << " (buffer: " << buffer_.size() << "/" << capacity_ << ")\n";
            
            // Simulate production time
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    Task<void> ProducerConsumerCoroutines::consumer_task(const std::string& consumer_name, int message_count) {
        for (int i = 0; i < message_count; ++i) {
            auto msg_opt = co_await try_consume();
            
            if (msg_opt) {
                auto processing_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - msg_opt->timestamp
                );
                
                std::cout << "📨 " << consumer_name << " consumed: " << msg_opt->content 
                          << " (processing time: " << processing_time.count() << "ms)\n";
                
                total_consumed_++;
            }
            
            // Simulate consumption time
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    }

    void ProducerConsumerCoroutines::start_demo(int num_producers, int num_consumers, int messages_per_producer) {
        std::cout << "🏭 Starting Coroutine Producer-Consumer Demo\n";
        
        std::vector<std::thread> threads;
        
        // Start producers
        for (int i = 0; i < num_producers; ++i) {
            threads.emplace_back([this, i, messages_per_producer]() {
                auto task = producer_task("Producer-" + std::to_string(i), messages_per_producer);
                // In a real implementation, we'd use a coroutine scheduler
                while (!task.is_ready()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
        }
        
        // Start consumers
        int messages_per_consumer = (num_producers * messages_per_producer) / num_consumers;
        for (int i = 0; i < num_consumers; ++i) {
            threads.emplace_back([this, i, messages_per_consumer]() {
                auto task = consumer_task("Consumer-" + std::to_string(i), messages_per_consumer);
                // In a real implementation, we'd use a coroutine scheduler
                while (!task.is_ready()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
    }

    void ProducerConsumerCoroutines::print_statistics() const {
        std::cout << "\n📊 === COROUTINE PRODUCER-CONSUMER STATISTICS ===\n";
        std::cout << "Messages Produced: " << total_produced_.load() << std::endl;
        std::cout << "Messages Consumed: " << total_consumed_.load() << std::endl;
        std::cout << "Buffer Size: " << buffer_.size() << "/" << capacity_ << std::endl;
        std::cout << "===============================================\n\n";
    }

    bool ProducerConsumerCoroutines::MessageAwaitable::await_ready() const {
        std::lock_guard<std::mutex> lock(parent_->buffer_mutex_);
        return !parent_->buffer_.empty();
    }

    void ProducerConsumerCoroutines::MessageAwaitable::await_suspend(std::coroutine_handle<> coro) {
        std::unique_lock<std::mutex> lock(parent_->buffer_mutex_);
        parent_->not_empty_.wait(lock, [this] { return !parent_->buffer_.empty(); });
        
        if (!parent_->buffer_.empty()) {
            message_ = parent_->buffer_.front();
            parent_->buffer_.pop();
            parent_->not_full_.notify_one();
        }
        
        coro.resume();
    }

    ProducerConsumerCoroutines::MessageAwaitable ProducerConsumerCoroutines::try_consume() {
        return MessageAwaitable{this};
    }

    // ========== WebCrawler Implementation ==========

    WebCrawler::WebCrawler(size_t max_concurrent_requests) : max_concurrent_(max_concurrent_requests) {}

    Task<WebCrawler::CrawlResult> WebCrawler::crawl_page(const std::string& url) {
        // Wait for available slot
        while (active_requests_.load() >= max_concurrent_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        active_requests_++;
        
        auto start_time = std::chrono::steady_clock::now();
        auto response = co_await network_client_.get_async(url);
        
        CrawlResult result = simulate_page_crawl(url);
        result.processing_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time
        );
        
        if (response.success) {
            result.found_links = extract_links(url, response.body);
            result.word_count = std::count(response.body.begin(), response.body.end(), ' ') + 1;
            result.success = true;
        } else {
            result.success = false;
            result.error = "Failed to fetch: " + std::to_string(response.status_code);
        }
        
        active_requests_--;
        co_return result;
    }

    Task<std::vector<WebCrawler::CrawlResult>> 
    WebCrawler::crawl_website(const std::string& base_url, int max_depth) {
        std::vector<CrawlResult> all_results;
        std::vector<std::string> urls_to_crawl = {base_url};
        
        for (int depth = 0; depth < max_depth && !urls_to_crawl.empty(); ++depth) {
            std::vector<std::string> next_level_urls;
            
            for (const auto& url : urls_to_crawl) {
                auto result = co_await crawl_page(url);
                all_results.push_back(result);
                
                if (result.success && depth < max_depth - 1) {
                    for (const auto& link : result.found_links) {
                        if (next_level_urls.size() < 10) { // Limit crawl scope
                            next_level_urls.push_back(link);
                        }
                    }
                }
            }
            
            urls_to_crawl = next_level_urls;
        }
        
        co_return all_results;
    }

    void WebCrawler::start_crawl_demo() {
        std::cout << "🕷️ Starting Web Crawler Demo\n";
        
        std::vector<std::string> seed_urls = {
            "https://example.com",
            "https://example.org",
            "https://example.net"
        };
        
        std::vector<std::thread> crawl_threads;
        
        for (const auto& url : seed_urls) {
            crawl_threads.emplace_back([this, url]() {
                // Simulate coroutine execution
                auto task = crawl_website(url, 2);
                // In a real implementation, we'd properly await the task
                std::this_thread::sleep_for(std::chrono::seconds(2));
                
                std::cout << "🔗 Completed crawling " << url << std::endl;
            });
        }
        
        for (auto& thread : crawl_threads) {
            thread.join();
        }
        
        std::cout << "✅ Web crawling demo completed\n";
    }

    WebCrawler::CrawlResult WebCrawler::simulate_page_crawl(const std::string& url) {
        CrawlResult result;
        result.url = url;
        return result; // Will be populated by crawl_page
    }

    std::vector<std::string> WebCrawler::extract_links(const std::string& url, const std::string& content) {
        // Simulate link extraction
        std::vector<std::string> links;
        
        // Generate some mock links based on the URL
        for (int i = 1; i <= 3; ++i) {
            links.push_back(url + "/page" + std::to_string(i));
        }
        
        return links;
    }

    // ========== DataProcessor Implementation ==========

    DataProcessor::DataProcessor() : scheduler_(2) {
        // Initialize processing pipeline
        pipeline_ = {
            {"Validate", [](const std::string& s) { return "[VALID] " + s; }, std::chrono::milliseconds(50)},
            {"Transform", [](const std::string& s) { 
                std::string result = s;
                std::transform(result.begin(), result.end(), result.begin(), ::toupper);
                return result;
            }, std::chrono::milliseconds(100)},
            {"Enrich", [](const std::string& s) { return s + " [ENRICHED]"; }, std::chrono::milliseconds(75)},
            {"Finalize", [](const std::string& s) { return "[FINAL] " + s; }, std::chrono::milliseconds(25)}
        };
    }

    Task<std::string> DataProcessor::process_data_async(const std::string& input) {
        std::string result = input;
        
        for (const auto& stage : pipeline_) {
            std::cout << "🔄 Processing stage: " << stage.name << std::endl;
            std::this_thread::sleep_for(stage.processing_time);
            result = stage.transform(result);
            
            // Yield to scheduler
            co_await scheduler_.yield();
        }
        
        co_return result;
    }

    Generator<std::string> DataProcessor::process_stream(const std::vector<std::string>& inputs) {
        for (const auto& input : inputs) {
            std::string result = input;
            
            for (const auto& stage : pipeline_) {
                std::this_thread::sleep_for(stage.processing_time);
                result = stage.transform(result);
            }
            
            co_yield result;
        }
    }

    AsyncGenerator<std::string> DataProcessor::process_stream_async(const std::vector<std::string>& inputs) {
        for (const auto& input : inputs) {
            // Simulate async processing
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            
            std::string result = input;
            for (const auto& stage : pipeline_) {
                result = stage.transform(result);
            }
            
            co_yield result;
        }
    }

    void DataProcessor::demonstrate_pipeline() {
        std::cout << "🏗️ Data Processing Pipeline Demo\n";
        
        scheduler_.start();
        
        std::vector<std::string> test_data = {"data1", "data2", "data3"};
        
        // Process using generator
        std::cout << "\n📊 Stream Processing Results:\n";
        auto generator = process_stream(test_data);
        
        for (const auto& result : generator) {
            std::cout << "✅ Processed: " << result << std::endl;
        }
        
        scheduler_.stop();
    }

    // ========== BasicCoroutinesDemo Implementation ==========

    void BasicCoroutinesDemo::demonstrate_generators() {
        std::cout << "=== Generator Coroutines ===\n";
        
        // Fibonacci generator
        std::cout << "Fibonacci sequence (first 10 numbers):\n";
        auto fib_gen = fibonacci_generator(10);
        
        for (const auto& num : fib_gen) {
            std::cout << num << " ";
        }
        std::cout << std::endl;
        
        // String generator
        std::vector<std::string> strings = {"Hello", "World", "From", "Coroutines"};
        std::cout << "\nString generator:\n";
        auto str_gen = string_generator(strings);
        
        for (const auto& str : str_gen) {
            std::cout << str << " ";
        }
        std::cout << std::endl << std::endl;
    }

    void BasicCoroutinesDemo::demonstrate_tasks() {
        std::cout << "=== Task Coroutines ===\n";
        
        // Factorial task
        auto factorial_task = compute_factorial(5);
        std::cout << "Computing factorial of 5...\n";
        
        // Wait for completion (in real implementation, would use proper scheduling)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        if (factorial_task.is_ready()) {
            try {
                int result = factorial_task.get();
                std::cout << "Factorial result: " << result << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Error: " << e.what() << std::endl;
            }
        }
        
        // String operation task
        auto string_task = async_string_operation("hello coroutines");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        if (string_task.is_ready()) {
            try {
                std::string result = string_task.get();
                std::cout << "String operation result: " << result << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Error: " << e.what() << std::endl;
            }
        }
        
        std::cout << std::endl;
    }

    void BasicCoroutinesDemo::demonstrate_async_generators() {
        std::cout << "=== Async Generator Coroutines ===\n";
        
        auto async_gen = async_number_generator(1, 5);
        auto iter = async_gen.begin();
        
        std::cout << "Async number generation:\n";
        
        // Simulate async iteration
        for (int i = 0; i < 5; ++i) {
            // In real implementation, would properly await
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            std::cout << "Generated: " << iter.value() << std::endl;
        }
        
        std::cout << std::endl;
    }

    void BasicCoroutinesDemo::demonstrate_coroutine_scheduler() {
        std::cout << "=== Coroutine Scheduler ===\n";
        
        CoroutineScheduler scheduler(2);
        scheduler.start();
        
        std::cout << "Scheduler started with 2 worker threads\n";
        std::cout << "Pending tasks: " << scheduler.pending_tasks() << std::endl;
        
        // In a real implementation, we would schedule actual coroutines
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        scheduler.stop();
        std::cout << "Scheduler stopped\n\n";
    }

    Generator<int> BasicCoroutinesDemo::fibonacci_generator(int count) {
        int a = 0, b = 1;
        
        for (int i = 0; i < count; ++i) {
            if (i == 0) co_yield a;
            else if (i == 1) co_yield b;
            else {
                int next = a + b;
                a = b;
                b = next;
                co_yield next;
            }
        }
    }

    Generator<std::string> BasicCoroutinesDemo::string_generator(const std::vector<std::string>& strings) {
        for (const auto& str : strings) {
            co_yield str;
        }
    }

    AsyncGenerator<int> BasicCoroutinesDemo::async_number_generator(int start, int end) {
        for (int i = start; i <= end; ++i) {
            simulate_async_work(std::chrono::milliseconds(100));
            co_yield i;
        }
    }

    Task<int> BasicCoroutinesDemo::compute_factorial(int n) {
        if (n < 0) {
            throw std::invalid_argument("Factorial of negative number");
        }
        
        int result = 1;
        for (int i = 1; i <= n; ++i) {
            result *= i;
            simulate_async_work(std::chrono::milliseconds(10));
        }
        
        co_return result;
    }

    Task<std::string> BasicCoroutinesDemo::async_string_operation(const std::string& input) {
        simulate_async_work(std::chrono::milliseconds(50));
        
        std::string result = input;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        
        co_return "[PROCESSED] " + result;
    }

    Task<std::vector<int>> BasicCoroutinesDemo::parallel_computation(const std::vector<int>& inputs) {
        std::vector<int> results;
        results.reserve(inputs.size());
        
        for (int value : inputs) {
            simulate_async_work(std::chrono::milliseconds(25));
            results.push_back(value * value);
        }
        
        co_return results;
    }

    void BasicCoroutinesDemo::simulate_async_work(std::chrono::milliseconds duration) {
        std::this_thread::sleep_for(duration);
    }

    // ========== CoroutinesDemo Implementation ==========

    void CoroutinesDemo::demonstrate_basic_coroutines() {
        print_section_header("Basic Coroutines");
        
        BasicCoroutinesDemo::demonstrate_generators();
        BasicCoroutinesDemo::demonstrate_tasks();
        BasicCoroutinesDemo::demonstrate_async_generators();
        BasicCoroutinesDemo::demonstrate_coroutine_scheduler();
        
        print_section_footer();
    }

    void CoroutinesDemo::demonstrate_async_file_operations() {
        print_section_header("Async File Operations");
        
        std::vector<std::string> filenames = {
            "config.txt", "data.json", "error_file.txt", "readme.md"
        };
        
        std::cout << "📁 Reading multiple files asynchronously...\n";
        
        // Simulate async file reading
        std::thread file_reader([&filenames]() {
            // In real implementation, would properly handle coroutines
            for (const auto& filename : filenames) {
                auto file_data = AsyncFileReader::simulate_file_read(filename);
                
                if (file_data.success) {
                    std::cout << "✅ Read " << filename << ": " 
                              << file_data.content.substr(0, 30) << "...\n";
                } else {
                    std::cout << "❌ Failed to read " << filename << ": " 
                              << file_data.error_message << std::endl;
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
        
        file_reader.join();
        
        print_section_footer();
    }

    void CoroutinesDemo::demonstrate_network_coroutines() {
        print_section_header("Network Coroutines");
        
        NetworkClient client;
        
        std::vector<std::string> urls = {
            "https://api.example.com/users",
            "https://api.example.com/posts", 
            "https://api.example.com/comments"
        };
        
        std::cout << "🌐 Making batch HTTP requests...\n";
        
        std::thread network_thread([&client, &urls]() {
            for (const auto& url : urls) {
                auto response = client.simulate_http_request(url);
                
                if (response.success) {
                    std::cout << "✅ " << url << " -> " << response.status_code 
                              << " (" << response.latency.count() << "ms)\n";
                } else {
                    std::cout << "❌ " << url << " -> " << response.status_code 
                              << " ERROR\n";
                }
            }
        });
        
        network_thread.join();
        
        print_section_footer();
    }

    void CoroutinesDemo::demonstrate_producer_consumer_coroutines() {
        print_section_header("Producer-Consumer Coroutines");
        
        ProducerConsumerCoroutines demo(5);
        demo.start_demo(2, 3, 8);
        demo.print_statistics();
        
        print_section_footer();
    }

    void CoroutinesDemo::demonstrate_web_crawler() {
        print_section_header("Web Crawler Coroutines");
        
        WebCrawler crawler(3);
        crawler.start_crawl_demo();
        
        print_section_footer();
    }

    void CoroutinesDemo::demonstrate_data_processing_pipeline() {
        print_section_header("Data Processing Pipeline");
        
        DataProcessor processor;
        processor.demonstrate_pipeline();
        
        print_section_footer();
    }

    void CoroutinesDemo::run_all_demonstrations() {
        std::cout << "\n🎯 ================================================\n";
        std::cout << "🎯 COMPREHENSIVE C++20 COROUTINES DEMONSTRATIONS\n";
        std::cout << "🎯 ================================================\n\n";
        
        demonstrate_basic_coroutines();
        demonstrate_async_file_operations();
        demonstrate_network_coroutines();
        demonstrate_producer_consumer_coroutines();
        demonstrate_web_crawler();
        demonstrate_data_processing_pipeline();
        
        std::cout << "\n🎉 ====================================\n";
        std::cout << "🎉 ALL COROUTINE DEMONSTRATIONS COMPLETED!\n";
        std::cout << "🎉 ====================================\n\n";
    }

    void CoroutinesDemo::print_section_header(const std::string& title) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "🔄 " << title << std::endl;
        std::cout << std::string(60, '=') << std::endl << std::endl;
    }

    void CoroutinesDemo::print_section_footer() {
        std::cout << std::string(60, '-') << std::endl;
        std::cout << "✅ Section Complete\n" << std::endl;
    }

    void CoroutinesDemo::simulate_work(std::chrono::milliseconds duration) {
        std::this_thread::sleep_for(duration);
    }

} // namespace CppVerseHub::Concurrency