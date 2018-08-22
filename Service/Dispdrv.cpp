/* nsemu - LGPL - Copyright 2018 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
#include "IpcStubs.hpp"
#include "Dispdrv.hpp"
namespace NVFlinger {

Display::Display(uint64_t id, std::string name) : id(id), name(name) {
        vsync_event = new Kernel::Event();
}

Layer::Layer(uint64_t id, BufferQueue* queue) : id(id), buffer_queue(queue) {}

static std::vector<Display> displays;

void Init() {
        displays.push_back(Display(0, "Default"));
}

uint64_t OpenDisplay(const std::string name) {
        ns_print("Opening display %s\n", name.c_str());
        auto itr = std::find_if(displays.begin(), displays.end(),
                        [&](const Display& display) { return display.name == name; });
        if (itr == displays.end()) {
                ns_abort("Cannnot find display\n");
        }
        return itr->id;
}

Display GetDisplay(uint64_t display_id) {
        auto itr = std::find_if(displays.begin(), displays.end(),
                                [&](const Display& display) { return display.id == display_id; });
        return *itr;
}

Layer GetLayer(uint64_t display_id, uint64_t layer_id) {
        auto display = GetDisplay(display_id);

        auto itr = std::find_if(display.layers.begin(), display.layers.end(),
                                [&](const Layer& layer) { return layer.id == layer_id; });
        return *itr;
}

Kernel::Event* GetVsyncEvent(uint64_t display_id) {
        auto display = GetDisplay(display_id);
        return display.vsync_event;
}

};
