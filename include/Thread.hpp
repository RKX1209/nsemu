#ifndef _THREAD_HPP
#define _THREAD_HPP

class KObject;
class Thread : public KObject {
public:
        uint32_t handle;
        Thread() : KObject() { }
};

namespace ThreadManager {
void Init();
Thread *Create();
}

#endif
