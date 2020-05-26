#ifndef CHIPPUHACHI_GPU_H
#define CHIPPUHACHI_GPU_H

#include <cstdint>

class gpu {
    static unsigned int const MAX_VIDEO_MEMORY = 2048;

    unsigned short graphics_memory[MAX_VIDEO_MEMORY];

public:
    gpu() = default;
    void init();
    void clear();
    void write(unsigned short address, unsigned short value);
};


#endif //CHIPPUHACHI_GPU_H
