#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <assert.h>

template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue(std::size_t capacity): _capacity(capacity) {
        assert(_capacity > 0);
    }

    void push(T t) {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_queue.size() >= _capacity) {
            _producer_cond.wait(lock, [&]() {
                return _queue.size() < _capacity;
            });
        }
        _queue.push(t);
        _consumer_cond.notify_one();
    }

    void pop(T& t) {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_queue.empty()) {
            _consumer_cond.wait(lock, [&]() {
                return !_queue.empty();
            });
        }
        t = _queue.front();
        _queue.pop();
        _producer_cond.notify_one();
    }

private:
    std::size_t _capacity;
    std::queue<T> _queue;
    std::mutex _mutex;
    std::condition_variable _consumer_cond;
    std::condition_variable _producer_cond;
};

int main() {
    std::size_t _buffer_size = 10;
    std::shared_ptr<ThreadSafeQueue<int>> p = std::make_shared<ThreadSafeQueue<int>>(_buffer_size);
    std::vector<std::thread> _producers;
    for (auto i = 0; i < _buffer_size; i++) {
        _producers.push_back(std::thread([&p, i]() {
            std::cout << "queue push " + std::to_string(i) + "\n";
            p->push(i);
        }));
    }
    std::thread _consumer([&p, _buffer_size]() {
        for(auto i = 0; i < _buffer_size; i++) {
            int res;
            p->pop(res);
            std::cout << "queue pop " + std::to_string(res) + "\n";
        }
    });
    for (auto& _producer : _producers) {
        _producer.join();
    }
    _consumer.join();
    return 0;
}