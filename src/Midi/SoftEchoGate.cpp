#include "Midi/SoftEchoGate.h"

#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace
{
bool envTokenIsTruthy(const char* value) noexcept
{
    if (value == nullptr || value[0] == '\0')
    {
        return false;
    }
    if (std::strcmp(value, "1") == 0)
    {
        return true;
    }
    char lower[8] = {};
    const std::size_t len = std::strlen(value);
    if (len >= sizeof(lower))
    {
        return false;
    }
    for (std::size_t index = 0; index < len; ++index)
    {
        lower[index] = static_cast<char>(std::tolower(static_cast<unsigned char>(value[index])));
    }
    return std::strcmp(lower, "true") == 0 || std::strcmp(lower, "yes") == 0;
}
} // namespace

bool softEchoEnvRequestsEnable() noexcept
{
    const char* value = std::getenv("UNITOR_MIDI_SOFT_ECHO");
    return envTokenIsTruthy(value);
}

void configureSoftEchoGate(bool enabledFromCli, bool forceOffFromCli) noexcept
{
    if (forceOffFromCli)
    {
        softEchoEnabledFlag().store(false, std::memory_order_relaxed);
        if (enabledFromCli || softEchoEnvRequestsEnable())
        {
            std::cerr
                << "Bridge soft-echo forced OFF (--no-soft-echo overrides CLI/env)\n"
                << std::flush;
        }
        return;
    }

    const bool enabled = enabledFromCli || softEchoEnvRequestsEnable();
    softEchoEnabledFlag().store(enabled, std::memory_order_relaxed);
    if (enabled)
    {
        std::cerr
            << "Bridge soft-echo ON (lab software-loop only; not USB/DIN path)\n"
            << std::flush;
    }
}
