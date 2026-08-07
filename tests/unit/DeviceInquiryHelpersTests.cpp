// Catch2 unit tests for Device Inquiry / Identity Reply helpers.

#include <catch2/catch_test_macros.hpp>

#include "Device/DeviceSessionSupport.h"

#include <cstdint>

TEST_CASE("Universal Device Inquiry matcher accepts F0 7E 7F 06 01 F7", "[inquiry]")
{
    const uint8_t inquiry[] = {0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7};
    REQUIRE(isUniversalDeviceInquiry(inquiry, sizeof(inquiry)));
}

TEST_CASE("Universal Device Inquiry matcher rejects Identity Reply", "[inquiry]")
{
    const uint8_t reply[] = {
        0xF0, 0x7E, 0x00, 0x06, 0x02, 0x10, 0x06, 0x00, 0x02, 0x00, 0x20, 0x31, 0x32, 0x30, 0xF7};
    REQUIRE_FALSE(isUniversalDeviceInquiry(reply, sizeof(reply)));
    REQUIRE(isIdentityReply(reply, sizeof(reply)));
}

TEST_CASE("Identity Reply matcher rejects Inquiry", "[inquiry]")
{
    const uint8_t inquiry[] = {0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7};
    REQUIRE_FALSE(isIdentityReply(inquiry, sizeof(inquiry)));
}

TEST_CASE("Inquiry helpers reject null or short buffers", "[inquiry]")
{
    const uint8_t shortBuf[] = {0xF0, 0x7E, 0x7F};
    REQUIRE_FALSE(isUniversalDeviceInquiry(nullptr, 6));
    REQUIRE_FALSE(isUniversalDeviceInquiry(shortBuf, sizeof(shortBuf)));
    REQUIRE_FALSE(isIdentityReply(nullptr, 5));
    REQUIRE_FALSE(isIdentityReply(shortBuf, sizeof(shortBuf)));
}

TEST_CASE("formatMidiBytesHex renders compact uppercase hex", "[inquiry]")
{
    const uint8_t reply[] = {0xF0, 0x7E, 0x00, 0x06, 0x02};
    REQUIRE(formatMidiBytesHex(reply, sizeof(reply)) == "F0 7E 00 06 02");
    REQUIRE(formatMidiBytesHex(nullptr, 3).empty());
    REQUIRE(formatMidiBytesHex(reply, 0).empty());
}
