// Synthetic EmagicCableMapper encode/decode smoke (--test-mapper).

#include "App/MapperSmoke.h"
#include "App/FramerSmoke.h"

#include "Protocol/EmagicMapperSmokeSupport.h"

#include <iostream>

int runMapperTests()
{
    if (!runAllEmagicMapperSmokeTests(std::cout, std::cerr))
    {
        return 1;
    }
    return runFramerTests() ? 0 : 1;
}
