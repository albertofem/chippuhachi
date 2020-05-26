#include <spdlog/spdlog.h>
#include "cpu.h"

void cpu::init(mem *memory_t, class gpu *gpu_t) {
    memory = memory_t;
    gpu = gpu_t;
}

bool cpu::cycle() {
    auto opcode = memory->read(program_counter << 8) + memory->read(program_counter + 1);

    switch (opcode & 0xF000) {
        case 0x0000:
            return handlex0000(opcode & 0x000F);

        case 0x1000:
            return handlex1000(opcode);

        case 0x2000:
            return handlex2000(opcode);

        case 0x3000:
            return handlex3000(opcode);

        case 0x4000:
            return handlex4000(opcode);

        case 0x5000:
            return handlex5000(opcode);

        case 0x6000:
            return handlex6000(opcode);

        case 0x7000:
            return handlex7000(opcode);

        case 0x8000:
            return handlex8000(opcode);

        case 0x9000:
            return handlex9000(opcode);

        case 0xA000:
            return handlexA000(opcode);

        case 0xB000:
            return handlexB000(opcode);

        case 0xC000:
            return handlexC000(opcode);

        case 0xD000:
            return handlexD000(opcode);

        case 0xE000:
            return handlexE000(opcode);

        case 0xF000:
            return handlexF000(opcode);

    }
}

bool cpu::handlex0000(unsigned short opcode) {
    switch (opcode) {
        case 0x0000:
            gpu->clear();
            program_counter += 2;
            return true;

        case 0x000E:
            --stack_pointer;
            program_counter = stack[stack_pointer];
            program_counter += 2;
            break;

        default:
            spdlog::error("Unknown opcode: {}", opcode);
            return false;
    }

    return false;
}

bool cpu::handlex1000(unsigned short opcode) {
    program_counter = opcode & 0x0FFF;
    return false;
}

bool cpu::handlex2000(unsigned short opcode) {
    stack[stack_pointer] = program_counter;
    ++stack_pointer;
    program_counter = opcode & 0x0FFF;

    return false;
}

bool cpu::handlex3000(unsigned short opcode) {
    if (video_register[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
        program_counter += 4;
    } else {
        program_counter += 2;
    }

    return false;
}

bool cpu::handlex4000(unsigned short opcode) {
    if (video_register[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
        program_counter += 4;
    } else {
        program_counter += 2;
    }

    return false;
}

bool cpu::handlex5000(unsigned short opcode) {
    return false;
}

bool cpu::handlex6000(unsigned short opcode) {
    return false;
}

bool cpu::handlex7000(unsigned short opcode) {
    return false;
}

bool cpu::handlex8000(unsigned short opcode) {
    return false;
}

bool cpu::handlex9000(unsigned short opcode) {
    return false;
}

bool cpu::handlexA000(unsigned short opcode) {
    return false;
}

bool cpu::handlexB000(unsigned short opcode) {
    return false;
}

bool cpu::handlexC000(unsigned short opcode) {
    return false;
}

bool cpu::handlexD000(unsigned short opcode) {
    return false;
}

bool cpu::handlexE000(unsigned short opcode) {
    return false;
}

bool cpu::handlexF000(unsigned short opcode) {
    return false;
}
