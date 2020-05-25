#ifndef CHIPPUHACHI_CHIPPUHACHI_H
#define CHIPPUHACHI_CHIPPUHACHI_H


#include "mem.h"
#include "cpu.h"
#include "gpu.h"
#include "backend/videobackend.h"
#include "backend/glfwvulkan.h"

class chippuhachi {
    mem *mem = new class mem();
    cpu *cpu = new class cpu();
    gpu *gpu = new class gpu();

    videobackend *backend = new glfwvulkan();
public:
    chippuhachi();
    void init();

    int run();

    int startVideo();
};


#endif //CHIPPUHACHI_CHIPPUHACHI_H
