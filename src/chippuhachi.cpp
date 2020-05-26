#include <spdlog/spdlog.h>
#include "chippuhachi.h"
#include <spdlog/sinks/stdout_color_sinks.h>

chippuhachi::chippuhachi() = default;

void chippuhachi::init() {
    spdlog::stdout_color_mt("c8");
    spdlog::get("c8")->info("Running　チップ８!");

    mem->init();
    gpu->init();

    cpu->init(mem, gpu);
}

bool chippuhachi::step() {
    if (!started)
    {
        return false;
    }

    if (!romLoaded)
    {
        return false;
    }

    auto mustDraw = cpu->cycle();

    return mustDraw;
}

bool chippuhachi::loadRom(const char *file_path) {
    auto result = mem->loadRom(file_path);

    if (!result)
    {
        spdlog::get("c8")->error("Unable to load rom");
    }

    return result;
}

void chippuhachi::start() {
    started = true;
}
