#ifndef CHIPPUHACHI_MEM_H
#define CHIPPUHACHI_MEM_H


class mem {
    static const int MAX_MEMORY = 4096;
    unsigned char memory[MAX_MEMORY];
public:
    mem();
    void init();
    bool loadRom(const char *file_path);
};


#endif //CHIPPUHACHI_MEM_H
