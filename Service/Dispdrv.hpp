#ifndef _DISPDRV_HPP
#define _DISPDRV_HPP

namespace NVFlinger {

class BufferQueue {
public:
        BufferQueue(uint32_t id, uint64_t layer_id);
        struct Buffer {
                uint32_t width;
                uint32_t height;
                uint32_t format;
        };
private:
        std::vector<Buffer> queue;

};

struct Layer {
        Layer(uint64_t id, BufferQueue* queue);
        uint64_t id;
        BufferQueue* buffer_queue;
};

struct Display {
        Display(uint64_t id, std::string name);
        uint64_t id;
        std::string name;
        std::vector<Layer> layers;
        Kernel::Event *vsync_event;
};

void Init();
uint64_t OpenDisplay(const std::string name);
Kernel::Event* GetVsyncEvent(uint64_t display_id);

};
#endif
