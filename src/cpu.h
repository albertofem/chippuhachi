#ifndef CHIPPUHACHI_CPU_H
#define CHIPPUHACHI_CPU_H


#include <cstdint>
#include "mem.h"
#include "gpu.h"

class cpu {
    static unsigned short const VIDEO_REGISTER_SIZE = 16;
    static unsigned short const STACK_SIZE = 16;
    static unsigned short const KEYPAD_MEMORY_SIZE = 16;

    unsigned short op_code;
    unsigned char video_register[VIDEO_REGISTER_SIZE];
    unsigned short index_register;
    unsigned short program_counter;

    unsigned short stack[STACK_SIZE];
    unsigned short stack_pointer;

    unsigned short delay_timer;
    unsigned short sound_timer;

    unsigned short keypad[KEYPAD_MEMORY_SIZE];

    mem* memory;
    gpu* gpu;

    bool handlex0000(unsigned short opcode);
public:
    void init(class mem* memory, class gpu* gpu_t);

    bool cycle();

    bool handlex1000(unsigned short opcode);

    bool handlex2000(unsigned short opcode);

    bool handlexF000(unsigned short opcode);

    bool handlex3000(unsigned short opcode);

    bool handlex4000(unsigned short opcode);

    bool handlex5000(unsigned short opcode);

    bool handlex6000(unsigned short opcode);

    bool handlex7000(unsigned short opcode);

    bool handlex8000(unsigned short opcode);

    bool handlex9000(unsigned short opcode);

    bool handlexA000(unsigned short opcode);

    bool handlexB000(unsigned short opcode);

    bool handlexC000(unsigned short opcode);

    bool handlexD000(unsigned short opcode);

    bool handlexE000(unsigned short opcode);
};


#endif //CHIPPUHACHI_CPU_H
