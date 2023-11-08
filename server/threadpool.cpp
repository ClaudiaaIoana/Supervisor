#include "threadpool.h"

threadpool::threadpool(int num_threads): stop(false) {
        for (unsigned int i = 0; i < num_threads; ++i) {
            threads.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty()) {
                            return;
                        }
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
        }
    }

threadpool::~threadpool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
        condition.notify_all();
    }
    
    for (std::thread& thread : threads) {
        if(thread.joinable()) {
            thread.join();
        }
        
    }
}
