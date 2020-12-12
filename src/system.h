#ifndef CHIPPUHACHI_SYSTEM_H
#define CHIPPUHACHI_SYSTEM_H

#include <vector>

class system {
public:
    virtual void init() = 0;

    virtual bool step() = 0;

    virtual bool loadRom(const char *file_path) = 0;

    virtual void start() = 0;

    virtual unsigned short renderWidth() = 0;

    virtual unsigned short renderHeight() = 0;

    virtual std::vector<unsigned short> pixels() = 0;

    virtual void keyPressed(int key, int value) = 0;
};

#endif
