#pragma once
/**
 * @file modbus_client.h
 * @brief Lightweight Modbus TCP client for Solakon ONE devices.
 *
 * Pure C++20 implementation using POSIX sockets. No external dependencies.
 * Supports reading holding registers (FC 0x03), input registers (FC 0x04),
 * and convenience methods for U32, I32, U16, I16, and string values.
 *
 * Usage:
 * @code
 *   solakon::modbus::Client client;
 *   auto ec = client.connect("192.168.178.121", 502);
 *   if (ec) { /* handle error */ }
 *
 *   uint32_t value;
 *   ec = client.read_u32(39118, value);
 *   if (ec) { /* handle error */ }
 *
 *   client.disconnect();
 * @endcode
 */

#include <cstdint>
#include <cstring>
#include <string>
#include <optional>
#include <array>
#include <span>
#include <system_error>

namespace solakon::modbus {

/**
 * @brief Modbus TCP client for communicating with Solakon ONE devices.
 *
 * Handles connection management, protocol framing, CRC-16 calculation,
 * and data conversion (big-endian byte order as per Modbus specification).
 */
class Client {
public:
    Client() = default;
    ~Client();
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    /**
     * @brief Connect to a Modbus TCP server.
     * @param host IP address or hostname of the device.
     * @param port TCP port (default 502 for Modbus).
     * @return std::error_code indicating success or failure.
     */
    std::error_code connect(const std::string& host, uint16_t port = 502);

    /**
     * @brief Disconnect from the Modbus TCP server.
     */
    void disconnect();

    /**
     * @brief Check if a connection is established.
     * @return true if connected, false otherwise.
     */
    bool is_connected() const;

    /**
     * @brief Read holding registers (Modbus function code 0x03).
     * @param start_addr Starting register address.
     * @param count Number of registers to read.
     * @param output Span to store the read values.
     * @return Number of registers read, or -1 on error.
     */
    int read_holding_registers(uint16_t start_addr, uint16_t count, std::span<uint16_t> output);

    /**
     * @brief Read input registers (Modbus function code 0x04).
     * @param start_addr Starting register address.
     * @param count Number of registers to read.
     * @param output Span to store the read values.
     * @return Number of registers read, or -1 on error.
     */
    int read_input_registers(uint16_t start_addr, uint16_t count, std::span<uint16_t> output);

    /**
     * @brief Read a 32-bit unsigned integer (2 consecutive holding registers).
     * @param addr Starting register address.
     * @param value Output variable for the read value.
     * @return std::error_code indicating success or failure.
     *
     * Combines two 16-bit registers as big-endian U32.
     */
    std::error_code read_u32(uint16_t addr, uint32_t& value);

    /**
     * @brief Read a 32-bit signed integer (2 consecutive holding registers).
     * @param addr Starting register address.
     * @param value Output variable for the read value.
     * @return std::error_code indicating success or failure.
     *
     * Combines two 16-bit registers as big-endian I32.
     */
    std::error_code read_i32(uint16_t addr, int32_t& value);

    /**
     * @brief Read a 16-bit unsigned integer (1 holding register).
     * @param addr Register address.
     * @param value Output variable for the read value.
     * @return std::error_code indicating success or failure.
     */
    std::error_code read_u16(uint16_t addr, uint16_t& value);

    /**
     * @brief Read a 16-bit signed integer (1 holding register).
     * @param addr Register address.
     * @param value Output variable for the read value.
     * @return std::error_code indicating success or failure.
     */
    std::error_code read_i16(uint16_t addr, int16_t& value);

    /**
     * @brief Read a string from consecutive holding registers.
     * @param addr Starting register address.
     * @param reg_count Number of registers to read.
     * @param result Output string (ASCII, big-endian per register).
     * @return std::error_code indicating success or failure.
     */
    std::error_code read_string(uint16_t addr, uint16_t reg_count, std::string& result);

private:
    int socket_fd_ = -1;
    uint16_t transaction_id_ = 1;

    /**
     * @brief Modbus TCP request frame structure.
     *
     * Transaction ID | Protocol ID | Length | Unit ID | Function Code | Start Addr | Quantity
     */
    struct MBPRequest {
        uint16_t transaction_id;
        uint16_t protocol_id = 0;
        uint16_t length;
        uint8_t unit_id;
        uint8_t function_code;
        uint16_t start_addr;
        uint16_t quantity;
    } __attribute__((packed));

    /**
     * @brief Modbus TCP response frame structure.
     *
     * Transaction ID | Protocol ID | Length | Unit ID | Function Code | Byte Count | Data
     */
    struct MBPResponse {
        uint16_t transaction_id;
        uint16_t protocol_id;
        uint16_t length;
        uint8_t unit_id;
        uint8_t function_code;
        uint8_t byte_count;
        std::array<uint8_t, 512> data;
    } __attribute__((packed));

    /**
     * @brief Send request and receive response over TCP socket.
     * @param request Request data buffer.
     * @param req_len Request buffer length.
     * @param response Response data buffer.
     * @param resp_len Response buffer length.
     * @return std::error_code indicating success or failure.
     */
    std::error_code send_receive(const uint8_t* request, size_t req_len,
                                  uint8_t* response, size_t resp_len);

    /**
     * @brief Calculate Modbus CRC-16.
     * @param data Data buffer.
     * @param len Data length in bytes.
     * @return CRC-16 checksum value.
     */
    static uint16_t modbus_crc16(const uint8_t* data, size_t len);

    /**
     * @brief Set socket to non-blocking mode with timeout.
     * @param fd File descriptor.
     * @param timeout_ms Timeout in milliseconds.
     * @return 0 on success, -1 on error.
     */
    static int make_nonblocking(int fd, int timeout_ms);

    /**
     * @brief Set standard socket options (reuse address, no delay).
     * @param fd File descriptor.
     */
    static void set_socket_options(int fd);
};

} // namespace solakon::modbus
