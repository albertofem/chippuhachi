#ifndef CHIPPUHACHI_GPU_H
#define CHIPPUHACHI_GPU_H

#include <cstdint>
#include <vector>

class gpu {
public:
    static unsigned int const WIDTH = 64;
    static unsigned int const HEIGHT = 32;

    gpu() = default;
    void init();
    void clear();
    void write(unsigned short address, unsigned short value);
    unsigned short read(unsigned short address);

    std::vector<unsigned short> pixels();
private:
    static unsigned int const MAX_VIDEO_MEMORY = WIDTH * HEIGHT;
    std::vector<unsigned short> graphics_memory;
};


#endif //CHIPPUHACHI_GPU_H
