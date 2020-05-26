#include "gpu.h"

void gpu::init() {

}

void gpu::clear() {
    for (unsigned short &i : graphics_memory) {
        i = 0;
    }
}
