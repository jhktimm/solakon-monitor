#pragma once
/**
 * @file network_scanner.h
 * @brief Network scanner for discovering Solakon ONE devices on the local network.
 *
 * Scans a subnet by TCP-connecting to configured ports (502=Modbus, 443=HTTPS).
 * Reports discovered devices with their IP, open ports, and response info.
 *
 * Usage:
 * @code
 *   solakon::scanner::NetworkScanner scanner;
 *   auto results = scanner.scan("192.168.178.0/24", {502, 443}, 2000);
 *   for (auto& r : results) {
 *       printf("Found: %s ports: ", r.ip.c_str());
 *       for (auto p : r.ports) printf("%d ", p);
 *       printf("\n");
 *   }
 * @endcode
 */

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

namespace solakon::scanner {

/**
 * @brief A discovered device on the network.
 */
struct DiscoveredDevice {
    std::string ip;           // IP address
    std::vector<uint16_t> ports;  // Open ports (502=Modbus, 443=HTTPS)
    std::string banner;       // Optional banner/response data
    bool is_solakon = false;  // Likely a Solakon device
};

/**
 * @brief Network scanner for Solakon ONE devices.
 */
class NetworkScanner {
public:
    NetworkScanner() = default;
    ~NetworkScanner() = default;

    /**
     * @brief Scan a subnet for open ports.
     * @param subnet CIDR notation (e.g., "192.168.178.0/24")
     * @param ports Ports to scan (e.g., {502, 443})
     * @param timeout_ms Timeout per connection attempt in milliseconds
     * @return Vector of discovered devices with open ports
     */
    std::vector<DiscoveredDevice> scan(
        const std::string& subnet,
        const std::vector<uint16_t>& ports,
        int timeout_ms = 2000);

    /**
     * @brief Extract hostnames from discovered devices.
     * @param devices Discovered devices
     * @return Vector of IP:port strings for display
     */
    static std::vector<std::string> format_results(const std::vector<DiscoveredDevice>& devices);

private:
    /**
     * @brief Parse CIDR notation into base IP and prefix length.
     * @param cidr CIDR string (e.g., "192.168.178.0/24")
     * @param base_ip Output: base IP address
     * @param prefix Output: prefix length (e.g., 24)
     * @return true if parsing succeeded
     */
    bool parse_cidr(const std::string& cidr, uint32_t& base_ip, uint8_t& prefix);

    /**
     * @brief Try TCP connecting to an IP:port.
     * @param ip IP address (network byte order)
     * @param port Port (host byte order)
     * @param timeout_ms Timeout in milliseconds
     * @return true if connection succeeded
     */
    bool try_connect(uint32_t ip, uint16_t port, int timeout_ms);

    /**
     * @brief Try to identify if a device is a Solakon ONE.
     * @param ip IP address (dotted string)
     * @param port Port that was open
     * @return true if device appears to be a Solakon ONE
     */
    bool identify_solakon(const std::string& ip, uint16_t port);
};

} // namespace solakon::scanner
