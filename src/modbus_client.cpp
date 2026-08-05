#include "modbus_client.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdexcept>
#include <vector>

namespace solakon::modbus {

Client::~Client() {
    disconnect();
}

std::error_code Client::connect(const std::string& host, uint16_t port) {
    if (is_connected()) {
        disconnect();
    }

    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        return std::error_code(errno, std::generic_category());
    }

    set_socket_options(socket_fd_);

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    if (::connect(socket_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        auto ec = std::error_code(errno, std::generic_category());
        ::close(socket_fd_);
        socket_fd_ = -1;
        return ec;
    }

    // NOTE: Socket bleibt BLOCKING. SO_SNDTIMEO/SO_RCVTIMEO (5s) in
    // set_socket_options() sorgen für den Timeout. Non-blocking + recv-Loop
    // funktioniert nicht zuverlässig, weil EAGAIN/EWOULDBLOCK als Fehler
    // interpretiert wird, obwohl Daten einfach noch nicht angekommen sind.
    return {};
}

void Client::disconnect() {
    if (socket_fd_ >= 0) {
        ::close(socket_fd_);
        socket_fd_ = -1;
    }
}

bool Client::is_connected() const {
    return socket_fd_ >= 0;
}

std::error_code Client::send_receive(const uint8_t* request, size_t req_len,
                                      uint8_t* response, size_t resp_len) {
    // Send request
    ssize_t sent = ::send(socket_fd_, request, req_len, MSG_NOSIGNAL);
    if (sent < 0) {
        return std::error_code(errno, std::generic_category());
    }

    // Receive response
    ssize_t total = 0;
    while (total < static_cast<ssize_t>(resp_len)) {
        ssize_t n = ::recv(socket_fd_, response + total, resp_len - total, 0);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Timeout - Modbus request failed
                return std::make_error_code(std::errc::timed_out);
            }
            return std::error_code(errno, std::generic_category());
        }
        if (n == 0) {
            return std::make_error_code(std::errc::connection_reset);
        }
        total += n;
    }
    return {};
}

std::error_code Client::read_u16(uint16_t addr, uint16_t& value) {
    uint16_t buf[1]{};
    int n = read_holding_registers(addr, 1, std::span(buf));
    if (n < 0) {
        return std::make_error_code(std::errc::io_error);
    }
    value = buf[0];
    return {};
}

std::error_code Client::read_i16(uint16_t addr, int16_t& value) {
    uint16_t buf[1]{};
    int n = read_holding_registers(addr, 1, std::span(buf));
    if (n < 0) {
        return std::make_error_code(std::errc::io_error);
    }
    value = static_cast<int16_t>(buf[0]);
    return {};
}

std::error_code Client::read_u32(uint16_t addr, uint32_t& value) {
    uint16_t buf[2]{};
    int n = read_holding_registers(addr, 2, std::span(buf));
    if (n < 0) {
        return std::make_error_code(std::errc::io_error);
    }
    // Big-endian (Modbus standard)
    value = (static_cast<uint32_t>(buf[0]) << 16) | buf[1];
    return {};
}

std::error_code Client::read_i32(uint16_t addr, int32_t& value) {
    uint32_t uval;
    auto ec = read_u32(addr, uval);
    if (!ec) {
        value = static_cast<int32_t>(uval);
    }
    return ec;
}

std::error_code Client::read_string(uint16_t addr, uint16_t reg_count, std::string& result) {
    std::vector<uint16_t> buf(reg_count);
    int n = read_holding_registers(addr, reg_count, std::span(buf));
    if (n < 0) {
        return std::make_error_code(std::errc::io_error);
    }
    // Convert registers to ASCII string (big-endian)
    result.clear();
    for (uint16_t reg : buf) {
        result += static_cast<char>((reg >> 8) & 0xFF);
        result += static_cast<char>(reg & 0xFF);
    }
    // Trim trailing nulls and spaces
    while (!result.empty() && (result.back() == '\0' || result.back() == ' ')) {
        result.pop_back();
    }
    return {};
}

int Client::read_holding_registers(uint16_t start_addr, uint16_t count, std::span<uint16_t> output) {
    if (count == 0 || output.size() < count) {
        return -1;
    }

    // Build Modbus TCP request (FC 0x03 - Read Holding Registers)
    MBPRequest req{};
    req.transaction_id = htons(transaction_id_++);
    req.protocol_id = htons(0);
    req.unit_id = 1;
    req.function_code = 0x03;
    req.start_addr = htons(start_addr);
    req.quantity = htons(count);
    req.length = htons(6);

    size_t req_len = sizeof(req);

    // Calculate response size
    uint8_t byte_count = count * 2;
    size_t resp_len = 9 + byte_count; // header(9) + data

    std::vector<uint8_t> resp(resp_len);

    auto ec = send_receive(reinterpret_cast<uint8_t*>(&req), req_len, resp.data(), resp_len);
    if (ec) {
        return -1;
    }

    // Parse response
    MBPResponse* rsp = reinterpret_cast<MBPResponse*>(resp.data());
    if (rsp->function_code != 0x03) {
        return -1;
    }

    // Copy data
    const uint8_t* data = resp.data() + 9; // skip 9-byte header
    for (uint16_t i = 0; i < count; i++) {
        output[i] = (static_cast<uint16_t>(data[i * 2]) << 8) | data[i * 2 + 1];
    }

    return static_cast<int>(count);
}

uint16_t Client::modbus_crc16(const uint8_t* data, size_t len) {
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

int Client::make_nonblocking(int fd, int timeout_ms) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) return -1;
    return 0;
}

void Client::set_socket_options(int fd) {
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
    struct timeval tv{};
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

} // namespace solakon::modbus
