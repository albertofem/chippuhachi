#include <spdlog/spdlog.h>
#include "chippuhachi.h"
#include <spdlog/sinks/stdout_color_sinks.h>

chippuhachi::chippuhachi() = default;

int chippuhachi::run() {
    init();

    backend->init(1600, 1200, "chippuhachi");

    auto result = backend->run();

    if (result->isSuccess) {
        spdlog::info("Exiting succesfully!");
    } else {
        spdlog::error("Video backend error with code ({}): {}", result->errorCode, result->errorMessage);
        return -1;
    }

    return 0;
}

void chippuhachi::init() {
    auto console = spdlog::stdout_color_mt("c8");

    spdlog::get("c8")->info("Running　チップ８!");

    mem->init();
    cpu->init();
    gpu->init();
}
