#pragma once
/**
 * Lightweight Modbus TCP client for Solakon ONE
 * Pure C++20, no external dependencies (uses POSIX sockets)
 */

#include <cstdint>
#include <cstring>
#include <string>
#include <optional>
#include <array>
#include <span>
#include <system_error>

namespace solakon::modbus {

class Client {
public:
    Client() = default;
    ~Client();
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    // Connect to Solakon ONE at given IP:port
    std::error_code connect(const std::string& host, uint16_t port = 502);
    void disconnect();
    bool is_connected() const;

    // Read holding registers (function code 0x03)
    // Returns number of registers read, or -1 on error
    int read_holding_registers(uint16_t start_addr, uint16_t count, std::span<uint16_t> output);

    // Read input registers (function code 0x04)
    int read_input_registers(uint16_t start_addr, uint16_t count, std::span<uint16_t> output);

    // Read multiple registers in one transaction (for U32 values spanning 2 registers)
    std::error_code read_u32(uint16_t addr, uint32_t& value);
    std::error_code read_i32(uint16_t addr, int32_t& value);
    std::error_code read_u16(uint16_t addr, uint16_t& value);
    std::error_code read_i16(uint16_t addr, int16_t& value);
    std::error_code read_string(uint16_t addr, uint16_t reg_count, std::string& result);

private:
    int socket_fd_ = -1;
    uint16_t transaction_id_ = 1;

    // Modbus TCP protocol helpers
    struct MBPRequest {
        uint16_t transaction_id;
        uint16_t protocol_id = 0;
        uint16_t length;
        uint8_t unit_id;
        uint8_t function_code;
        uint16_t start_addr;
        uint16_t quantity;
    } __attribute__((packed));

    struct MBPResponse {
        uint16_t transaction_id;
        uint16_t protocol_id;
        uint16_t length;
        uint8_t unit_id;
        uint8_t function_code;
        uint8_t byte_count;
        std::array<uint8_t, 512> data;
    } __attribute__((packed));

    std::error_code send_receive(const uint8_t* request, size_t req_len,
                                  uint8_t* response, size_t resp_len);
    static uint16_t modbus_crc16(const uint8_t* data, size_t len);
    static int make_nonblocking(int fd, int timeout_ms);
    static void set_socket_options(int fd);
};

} // namespace solakon::modbus
