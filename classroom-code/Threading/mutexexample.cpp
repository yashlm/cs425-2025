#include <iostream>
#include <thread>
#include <mutex>

std::mutex mtx; // Shared mutex

void critical_section(int thread_id) {
    std::cout << "Thread " << thread_id << " trying to lock the mutex.\n";

    // Lock the mutex using std::lock_guard
    std::lock_guard<std::mutex> lock(mtx);

    // Critical section (only one thread can execute this at a time)
    std::cout << "Thread " << thread_id << " has locked the mutex.\n";
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Simulate work
    std::cout << "Thread " << thread_id << " is unlocking the mutex.\n";

    // The lock_guard is destroyed here, unlocking the mutex automatically
}

int main() {
    std::thread t1(critical_section, 1);
    std::thread t2(critical_section, 2);
    std::thread t3(critical_section, 3);

    t1.join();
    t2.join();
    t3.join();

    return 0;
}
 