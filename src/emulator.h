#ifndef CHIPPUHACHI_EMULATOR_H
#define CHIPPUHACHI_EMULATOR_H

#include "backend/videobackend.h"
#include "backend/glfwvulkan.h"
#include "chippuhachi.h"

class emulator {
    videobackend *backend = new glfwvulkan();
    class system* emulatedSystem;

public:
    int run();
};

#endif
