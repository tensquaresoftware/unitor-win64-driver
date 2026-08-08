// Catch2 unit tests for Device Inquiry / Identity Reply helpers.

#include <catch2/catch_test_macros.hpp>

#include "Device/DeviceSessionSupport.h"

#include <cstdint>
#include <vector>

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

TEST_CASE("leading-F0 repair prepends on lone 0x10 under expect", "[inquiry][f0-repair]")
{
    const uint8_t span[] = {0x10};
    std::vector<uint8_t> storage;
    const MidiPushView view = maybePrependLostLeadingF0(true, span, sizeof(span), storage);
    REQUIRE(view.count == 2);
    REQUIRE(view.bytes[0] == 0xF0);
    REQUIRE(view.bytes[1] == 0x10);
    REQUIRE(storage.size() == 2);
}

TEST_CASE("leading-F0 repair prepends on same-span 10 06 under expect", "[inquiry][f0-repair]")
{
    const uint8_t span[] = {0x10, 0x06, 0x01};
    std::vector<uint8_t> storage;
    const MidiPushView view = maybePrependLostLeadingF0(true, span, sizeof(span), storage);
    REQUIRE(view.count == 4);
    REQUIRE(view.bytes[0] == 0xF0);
    REQUIRE(view.bytes[1] == 0x10);
    REQUIRE(view.bytes[2] == 0x06);
    REQUIRE(view.bytes[3] == 0x01);
}

TEST_CASE("leading-F0 repair leaves span unchanged when expect idle", "[inquiry][f0-repair]")
{
    const uint8_t span[] = {0x10};
    std::vector<uint8_t> storage;
    const MidiPushView view = maybePrependLostLeadingF0(false, span, sizeof(span), storage);
    REQUIRE(view.bytes == span);
    REQUIRE(view.count == 1);
    REQUIRE(storage.empty());
}

TEST_CASE("leading-F0 repair ignores multi-byte span that is not 10 06", "[inquiry][f0-repair]")
{
    const uint8_t span[] = {0x10, 0x07};
    std::vector<uint8_t> storage;
    const MidiPushView view = maybePrependLostLeadingF0(true, span, sizeof(span), storage);
    REQUIRE(view.bytes == span);
    REQUIRE(view.count == 2);
    REQUIRE(storage.empty());
}

TEST_CASE("exact Matrix dump lengths are 275 and 351 only", "[inquiry][matrix-dump]")
{
    REQUIRE(isExactMatrixDumpLength(275));
    REQUIRE(isExactMatrixDumpLength(351));
    // Lab mid-burst shorts: missing N×32 USB packets under 275.
    REQUIRE_FALSE(isExactMatrixDumpLength(243));
    REQUIRE_FALSE(isExactMatrixDumpLength(211));
    REQUIRE_FALSE(isExactMatrixDumpLength(179));
    REQUIRE_FALSE(isExactMatrixDumpLength(319)); // 351 − 32
    REQUIRE_FALSE(isExactMatrixDumpLength(287)); // 351 − 64
    REQUIRE_FALSE(isExactMatrixDumpLength(274));
    REQUIRE_FALSE(isExactMatrixDumpLength(0));
}

TEST_CASE("Matrix dump reply matcher accepts patch and master prefixes", "[inquiry][matrix-dump]")
{
    const uint8_t patch[] = {0xF0, 0x10, 0x06, 0x01, 0x00};
    const uint8_t master[] = {0xF0, 0x10, 0x06, 0x03, 0x00};
    const uint8_t other[] = {0xF0, 0x10, 0x06, 0x04, 0x00};
    REQUIRE(isMatrixDumpReply(patch, sizeof(patch)));
    REQUIRE(isMatrixDumpReply(master, sizeof(master)));
    REQUIRE_FALSE(isMatrixDumpReply(other, sizeof(other)));
    REQUIRE_FALSE(isMatrixDumpReply(nullptr, 5));
}

TEST_CASE("Matrix dump size-reject retry budget is at least two", "[inquiry][matrix-dump]")
{
    REQUIRE(kMatrixDumpSizeRejectRetries >= 2);
}
