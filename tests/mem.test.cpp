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

    GIVEN("an invalid rom file (non existant)") {
        auto filePath = "idontexist";

        WHEN("the rom is loaded") {
            auto result = c8mem->loadRom(filePath);
            REQUIRE(result == false);
        }
    }
}