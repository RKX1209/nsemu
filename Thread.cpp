#include "Nsemu.hpp"

namespace ThreadManager {
std::unordered_map<uint32_t, Thread *> threads;
unsigned long thread_id;

void Init() {
        thread_id = 0;
}

Thread *Create() {
        Thread *thread = new Thread();
        threads[thread_id++] = thread;
        thread->handle = NewHandle(thread);
        return thread;
}

};
