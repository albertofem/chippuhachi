#ifndef CHIPPUHACHI_CPU_H
#define CHIPPUHACHI_CPU_H


class cpu {
    unsigned short opcode;
    unsigned char V[16];
    unsigned short I;
    unsigned short pc;
public:
    cpu() = default;

    void init();

    void cycle();
};


#endif //CHIPPUHACHI_CPU_H
