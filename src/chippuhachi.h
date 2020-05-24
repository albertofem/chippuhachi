#ifndef CHIPPUHACHI_CHIPPUHACHI_H
#define CHIPPUHACHI_CHIPPUHACHI_H


#include "mem.h"
#include "cpu.h"
#include "gpu.h"

class chippuhachi {
    mem *mem = new class mem();
    cpu *cpu = new class cpu();
    gpu *gpu = new class gpu();
public:
    chippuhachi();
    void init();

    int run();
};


#endif //CHIPPUHACHI_CHIPPUHACHI_H
