#include "emulator.h"

int main(int argc, char **argv)
{
    auto emu = new emulator();
    return emu->run();
}