#include <spdlog/spdlog.h>
#include "cpu.h"

void cpu::init(mem *memory_t, class gpu *gpu_t) {
    memory = memory_t;
    gpu = gpu_t;

    program_counter = 0x200;
    op_code = 0;
    stack_pointer = 0;
    index_register = 0;

    for (unsigned short &i : stack) {
        i = 0;
    }

    for (unsigned char &i : video_register) {
        i = 0;
    }

    for (unsigned short &i : keypad) {
        i = 0;
    }

    delay_timer = 0;
    sound_timer = 0;

    spdlog::get("c8")->info("Reset CPU. Program counter is: {0:x}", program_counter);
}

bool cpu::cycle() {
    unsigned short opcode = (memory->read(program_counter) << 8u) + memory->read(program_counter + 1u);

    switch (opcode & 0xF000u) {
        case 0x0000:
            return handlex0000(opcode & 0x000Fu);

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

    return false;
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
            spdlog::error("Unknown opcode: 0x{0:b} - 0x{0:x}", opcode, opcode);
            return false;
    }

    return false;
}

bool cpu::handlex1000(unsigned short opcode) {
    program_counter = opcode & 0x0FFFu;
    return false;
}

bool cpu::handlex2000(unsigned short opcode) {
    stack[stack_pointer] = program_counter;
    ++stack_pointer;
    program_counter = opcode & 0x0FFFu;

    return false;
}

bool cpu::handlex3000(unsigned short opcode) {
    if (video_register[(opcode & 0x0F00u) >> 8u] == (opcode & 0x00FFu)) {
        program_counter += 4;
    } else {
        program_counter += 2;
    }

    return false;
}

bool cpu::handlex4000(unsigned short opcode) {
    if (video_register[(opcode & 0x0F00u) >> 8u] != (opcode & 0x00FFu)) {
        program_counter += 4;
    } else {
        program_counter += 2;
    }

    return false;
}

bool cpu::handlex5000(unsigned short opcode) {
    if (video_register[(opcode & 0x0F00u) >> 8u] == (opcode & 0x00FFu)) {
        program_counter += 4;
    } else {
        program_counter += 2;
    }

    return false;
}

bool cpu::handlex6000(unsigned short opcode) {
    video_register[(opcode & 0x0F00u) >> 8u] = opcode & 0x00FFu;
    program_counter += 2;

    return false;
}

bool cpu::handlex7000(unsigned short opcode) {
    video_register[(opcode & 0x0F00u) >> 8u] += opcode & 0x00FFu;
    program_counter += 2;

    return false;
}

bool cpu::handlex8000(unsigned short opcode) {
    switch (opcode & 0x000Fu) {

        case 0x0000:
            video_register[(opcode & 0x0F00u) >> 8u] = video_register[(opcode & 0x00F0u) >> 4u];

        case 0x0001:
            video_register[(opcode & 0x0F00u) >> 8u] |= video_register[(opcode & 0x00F0u) >> 4u];

        case 0x0002:
            video_register[(opcode & 0x0F00u) >> 8u] &= video_register[(opcode & 0x00F0u) >> 4u];

        case 0x0003:
            video_register[(opcode & 0x0F00u) >> 8u] ^= video_register[(opcode & 0x00F0u) >> 4u];

        case 0x0004:
            video_register[(opcode & 0x0F00u) >> 8u] += video_register[(opcode & 0x00F0u) >> 4u];

            if (video_register[(opcode & 0x00F0u) >> 4u] > (0xFF - video_register[(opcode & 0x0F00u) >> 8u])) {
                video_register[0xF] = 1;
            } else {
                video_register[0xF] = 0;
            }

        case 0x0005:
            if (video_register[(opcode & 0x00F0u) >> 4u] > video_register[(opcode & 0x0F00u) >> 8u]) {
                video_register[0xF] = 0;
            } else {
                video_register[0xF] = 1;
            }

            video_register[(opcode & 0x0F00u) >> 8u] -= video_register[(opcode & 0x00F0u) >> 4u];

        case 0x0006:
            video_register[0xF] = video_register[(opcode & 0x0F00u) >> 8u] & 0x1u;
            video_register[(opcode & 0x0F00u) >> 8u] >>= 1u;

        case 0x0007:
            if (video_register[(opcode & 0x0F00u) >> 8u] > video_register[(opcode & 0x00F0u) >> 4u]) {
                video_register[0xF] = 0;
            } else {
                video_register[0xF] = 1;
            }

            video_register[(opcode & 0x0F00u) >> 8u] =
                    video_register[(opcode & 0x00F0u) >> 4u] - video_register[(opcode & 0x0F00u) >> 8u];

        case 0x000E:
            video_register[0xF] = video_register[(opcode & 0x0F00u) >> 8u] >> 7u;
            video_register[(opcode & 0x0F00u) >> 8u] <<= 1u;
    }

    program_counter += 2;
    return false;
}

bool cpu::handlex9000(unsigned short opcode) {
    if (video_register[(opcode & 0x0F00u) >> 8u] != video_register[(opcode & 0x00F0u) >> 4u]) {
        program_counter += 4;
    } else {
        program_counter += 2;
    }

    return false;
}

bool cpu::handlexA000(unsigned short opcode) {
    index_register = opcode & 0x0FFFu;
    program_counter += 2;

    return false;
}

bool cpu::handlexB000(unsigned short opcode) {
    program_counter = (opcode & 0x0FFFu) + video_register[0];

    return false;
}

bool cpu::handlexC000(unsigned short opcode) {
    video_register[(opcode & 0x0F00u) >> 8u] = (rand() % (0xFFu + 1)) & (opcode & 0x00FFu);
    program_counter += 2;

    return false;
}

bool cpu::handlexD000(unsigned short opcode) {
    unsigned short x = video_register[(opcode & 0x0F00u) >> 8u];
    unsigned short y = video_register[(opcode & 0x00F0u) >> 4u];
    unsigned short height = opcode & 0x000Fu;
    unsigned short pixel;

    video_register[0xF] = 0;
    for (int yline = 0; yline < height; yline++) {
        pixel = memory->read(index_register + yline);
        for (unsigned short xline = 0; xline < 8; xline++) {
            if ((pixel & (0x80u >> xline)) != 0) {
                if (gpu->read((x + xline + ((y + yline) * 64))) == 1) {
                    video_register[0xF] = 1;
                }

                auto writeAddress = x + xline + ((y + yline) * 64);

                gpu->write(writeAddress, gpu->read(writeAddress) ^ 1u);
            }
        }
    }

    program_counter += 2;

    return true;
}

bool cpu::handlexE000(unsigned short opcode) {
    switch (opcode & 0x00FFu) {
        case 0x009E:
            if (keypad[video_register[(opcode & 0x0F00u) >> 8u]] != 0)
                program_counter += 4;
            else
                program_counter += 2;
            break;

        case 0x00A1:
            if (keypad[video_register[(opcode & 0x0F00u) >> 8u]] == 0)
                program_counter += 4;
            else
                program_counter += 2;
            break;

        default:
            spdlog::error("Unknown op code: {}", opcode);
    }

    return false;
}

bool cpu::handlexF000(unsigned short opcode) {
    switch (opcode & 0x00FFu) {
        case 0x0007:
            video_register[(opcode & 0x0F00u) >> 8u] = delay_timer;
            program_counter += 2;
            break;

        case 0x000A: {
            bool key_pressed = false;

            for (int i = 0; i < 16; ++i) {
                if (keypad[i] != 0) {
                    video_register[(opcode & 0x0F00u) >> 8u] = i;
                    key_pressed = true;
                }
            }

            if (!key_pressed) {
                return false;
            }

            program_counter += 2;
            break;
        }

        case 0x0015:
            delay_timer = video_register[(opcode & 0x0F00u) >> 8u];
            program_counter += 2;
            break;

        case 0x0018:
            sound_timer = video_register[(opcode & 0x0F00u) >> 8u];
            program_counter += 2;
            break;

        case 0x001E:
            if (index_register + video_register[(opcode & 0x0F00u) >> 8u] > 0xFFFu) {
                video_register[0xF] = 1;
            } else {
                video_register[0xF] = 0;
            }

            index_register += video_register[(opcode & 0x0F00u) >> 8u];
            program_counter += 2;

            break;

        case 0x0029:
            index_register = video_register[(opcode & 0x0F00u) >> 8u] * 0x5;
            program_counter += 2;

            break;

        case 0x0033:
            memory->write(index_register, video_register[(opcode & 0x0F00u) >> 8u] / 100);
            memory->write(index_register + 1, (video_register[(opcode & 0x0F00u) >> 8u] / 10) % 10);
            memory->write(index_register + 2, video_register[(opcode & 0x0F00u) >> 8u] % 10);

            program_counter += 2;

            break;

        case 0x0055:
            for (int i = 0; i <= ((opcode & 0x0F00u) >> 8u); ++i) {
                memory->write(index_register + i, video_register[i]);
            }

            index_register += ((opcode & 0x0F00u) >> 8u) + 1;
            program_counter += 2;
            break;

        case 0x0065:
            for (int i = 0; i <= ((opcode & 0x0F00u) >> 8u); ++i)
                video_register[i] = memory->read(index_register + i);

            index_register += ((opcode & 0x0F00u) >> 8u) + 1;
            program_counter += 2;
            break;

        default:
            spdlog::error("Unknown opcode [0xF000]: 0x%X\n", opcode);
    }

    return false;
}
