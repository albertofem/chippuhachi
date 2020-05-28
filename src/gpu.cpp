#include "gpu.h"

void gpu::init() {
    graphics_memory = std::vector<unsigned short>(MAX_VIDEO_MEMORY);
}

void gpu::clear() {
    graphics_memory = std::vector<unsigned short>(MAX_VIDEO_MEMORY);
}

void gpu::write(unsigned short address, unsigned short value) {
    graphics_memory[address] = value;
}

unsigned short gpu::read(unsigned short address)
{
    return graphics_memory[address];
}

std::vector<unsigned short> gpu::pixels() {
    return graphics_memory;
}
