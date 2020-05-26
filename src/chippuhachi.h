#ifndef CHIPPUHACHI_CHIPPUHACHI_H
#define CHIPPUHACHI_CHIPPUHACHI_H

#include "mem.h"
#include "cpu.h"
#include "gpu.h"
#include "backend/videobackend.h"
#include "backend/glfwvulkan.h"
#include "emulator.h"
#include "system.h"

class chippuhachi : public system {
    mem *mem = new class mem();
    cpu *cpu = new class cpu();
    gpu *gpu = new class gpu();

public:
    chippuhachi();
    void init() override;
    bool step() override;
};


#endif