#ifndef CHIPPUHACHI_GPU_H
#define CHIPPUHACHI_GPU_H

#include <cstdint>

class gpu {
    static unsigned int const MAX_VIDEO_MEMORY = 64 * 32;

    uint8_t graphics_memory[MAX_VIDEO_MEMORY];

public:
    gpu() = default;
    void init();
};


#endif //CHIPPUHACHI_GPU_H
