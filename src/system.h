#ifndef CHIPPUHACHI_SYSTEM_H
#define CHIPPUHACHI_SYSTEM_H

class system {
public:
    virtual void init() = 0;
    virtual bool step() = 0;
    virtual bool loadRom(const char* file_path) = 0;
    virtual void start() = 0;
};

#endif
