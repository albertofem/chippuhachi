#include <spdlog/spdlog.h>
#include "chippuhachi.h"
#include <spdlog/sinks/stdout_color_sinks.h>

chippuhachi::chippuhachi() = default;

void chippuhachi::init() {
    spdlog::stdout_color_mt("c8");
    spdlog::get("c8")->info("Running　チップ８!");

    mem->init();
    cpu->init();
    gpu->init();
}

bool chippuhachi::step() {
    return false;
}
