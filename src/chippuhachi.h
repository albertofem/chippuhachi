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

    bool started{};
    bool romLoaded{};

public:
    chippuhachi();
    void init() override;
    bool step() override;
    bool loadRom(const char *file_path) override;
    void start() override;

    unsigned short renderWidth() override;

    unsigned short renderHeight() override;

    std::vector<unsigned short> pixels() override;

    void keyPressed(int key, int value) override;
};


#endif