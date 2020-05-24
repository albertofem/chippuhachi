#include <catch2/catch.hpp>

#include "mem.h"

SCENARIO("rom can be loaded") {
    auto c8mem = new mem();
    GIVEN("a valid rom file") {
        auto filePath = "roms/guess";

        WHEN("the rom is loaded") {
            auto result = c8mem->loadRom(filePath);
            REQUIRE(result);
        }
    }
}