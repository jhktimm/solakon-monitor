#define CATCH_CONFIG_RUNNER
#include <catch2/catch_test_macros.hpp>
#include "test_helpers.hpp"

// CRC16 Modbus algorithm (same as in modbus_client.cpp)
static uint16_t crc16_modbus(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= static_cast<uint16_t>(data[i]);
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

TEST_CASE("CRC16 Modbus: empty input", "[modbus][crc]") {
    uint8_t data[1] = {0};
    REQUIRE(crc16_modbus(data, 0) == 0xFFFF);
}

TEST_CASE("CRC16 Modbus: single byte 0x01", "[modbus][crc]") {
    uint8_t data[] = {0x01};
    REQUIRE(crc16_modbus(data, 1) == 0x807E);
}

TEST_CASE("CRC16 Modbus: all zeros", "[modbus][crc]") {
    uint8_t data[] = {0x00, 0x00, 0x00};
    REQUIRE(crc16_modbus(data, 3) == 0xC071);
}

TEST_CASE("CRC16 Modbus: known Modbus request", "[modbus][crc]") {
    // Read holding registers: unit=1, func=0x03, start=0x0000, count=0x0001
    uint8_t data[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01};
    REQUIRE(crc16_modbus(data, 6) == 0x0A84);
}

TEST_CASE("CRC16 Modbus: idempotent on identical input", "[modbus][crc]") {
    uint8_t data[] = {0x10, 0x20, 0x30, 0x40, 0x50};
    uint16_t crc1 = crc16_modbus(data, 5);
    uint16_t crc2 = crc16_modbus(data, 5);
    REQUIRE(crc1 == crc2);
}

TEST_CASE("CRC16 Modbus: different input produces different CRC", "[modbus][crc]") {
    uint8_t data1[] = {0x01, 0x02, 0x03};
    uint8_t data2[] = {0x01, 0x02, 0x04};
    uint16_t crc1 = crc16_modbus(data1, 3);
    uint16_t crc2 = crc16_modbus(data2, 3);
    REQUIRE(crc1 != crc2);
}

TEST_CASE("CRC16 Modbus: byte order matters", "[modbus][crc]") {
    uint8_t data1[] = {0x01, 0x02, 0x03};
    uint8_t data2[] = {0x03, 0x02, 0x01};
    uint16_t crc1 = crc16_modbus(data1, 3);
    uint16_t crc2 = crc16_modbus(data2, 3);
    REQUIRE(crc1 != crc2);
}

TEST_CASE("CRC16 Modbus: larger payload", "[modbus][crc]") {
    uint8_t data[256];
    for (int i = 0; i < 256; i++) data[i] = static_cast<uint8_t>(i);
    uint16_t crc = crc16_modbus(data, 256);
    REQUIRE(crc == 0xDE6C);
}
