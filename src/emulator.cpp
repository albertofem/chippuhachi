#include <spdlog/spdlog.h>
#include "emulator.h"

int emulator::run() {
    emulatedSystem = new chippuhachi();

    backend->init(1600, 1200, "chippuhachi");
    emulatedSystem->init();

    auto result = backend->run(emulatedSystem);

    if (result->isSuccess) {
        spdlog::info("Exiting succesfully!");
    } else {
        spdlog::error("Video backend error with code ({}): {}", result->errorCode, result->errorMessage);
        return -1;
    }
}
