#ifndef CHIPPUHACHI_SYSTEM_H
#define CHIPPUHACHI_SYSTEM_H

class system {
public:
    virtual void init() = 0;
    virtual bool step() = 0;
};

#endif
