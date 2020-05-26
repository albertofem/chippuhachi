#include "gpu.h"

void gpu::init() {

}

void gpu::clear() {
    for (unsigned short &i : graphics_memory) {
        i = 0;
    }
}

void gpu::write(unsigned short address, unsigned short value) {
    graphics_memory[address] = value;
}
