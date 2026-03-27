#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <chrono>
#include <cstdlib>

#define BUFFER_SIZE 5

class ThreadSafeCircularBuffer {
private:
    int buffer[BUFFER_SIZE];
    int in = 0, out = 0, count = 0;
    std::mutex mtx;
    std::condition_variable not_full, not_empty;

public:
    void push(int value) {
        std::unique_lock<std::mutex> lock(mtx);
        not_full.wait(lock, [this] { return count < BUFFER_SIZE; });
        buffer[in] = value;
        in = (in + 1) % BUFFER_SIZE;
        count++;
        lock.unlock();
        not_empty.notify_one();
    }

    bool pop(int& value) {
        std::unique_lock<std::mutex> lock(mtx);
        not_empty.wait(lock, [this] { return count > 0; });
        value = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;
        lock.unlock();
        not_full.notify_one();
        return true;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mtx);
        return count == 0;
    }

    bool full() {
        std::lock_guard<std::mutex> lock(mtx);
        return count == BUFFER_SIZE;
    }
};

void circularBufferTest() {
    ThreadSafeCircularBuffer cb;
    std::vector<std::thread> producers, consumers;
    const int NUM_ITEMS = 10;

    auto producer = [&cb]() {
        for (int i = 0; i < NUM_ITEMS; ++i) {
            int value = rand() % 100;
            cb.push(value);
            std::cout << "Produced: " << value << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    };

    auto consumer = [&cb]() {
        for (int i = 0; i < NUM_ITEMS; ++i) {
            int value;
            cb.pop(value);
            std::cout << "Consumed: " << value << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    };

    producers.emplace_back(producer);
    consumers.emplace_back(consumer);

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();
}

int main() {
    circularBufferTest();
    return 0;
}
