#include <cstdio>
#include <iostream>
#include "mem.h"

void mem::init() {
    clearMemory();
    loadFontSet();
}

void mem::clearMemory() {
    for (unsigned char &i : memory) {
        i = 0;
    }
}

void mem::loadFontSet() {
    for (int i = 0; i < 80; ++i) {
        memory[i] = C8_FONTSET[i];
    }
}

bool mem::loadRom(const char *file_path) {
    FILE *rom = fopen(file_path, "rb");

    if (rom == nullptr) {
        std::cerr << "Unable to open rom: " << errno << std::endl;
        return false;
    }

    fseek(rom, 0, SEEK_END);
    long rom_size = ftell(rom);

    rewind(rom);

    char *rom_buffer = (char *) malloc(sizeof(char) * rom_size);

    if (rom_buffer == nullptr) {
        std::cerr << "Failed to allocate memory: " << errno << std::endl;
        return false;
    }

    size_t result = fread(rom_buffer, sizeof(char), (size_t) rom_size, rom);

    if (result != rom_size) {
        std::cerr << "Failed to read rom: " << errno << std::endl;
        return false;
    }

    if ((MAX_MEMORY - 512) > rom_size) {
        for (int i = 0; i < rom_size; ++i) {
            memory[i + 512] = (uint8_t) rom_buffer[i];
        }
    } else {
        std::cerr << "Rom too large to fit in memory" << std::endl;
        return false;
    }

    fclose(rom);
    free(rom_buffer);

    return true;
}

unsigned short mem::read(unsigned short address) {
    return memory[address];
}

void mem::write(unsigned short address, unsigned short value) {
    memory[address] = value;
}
