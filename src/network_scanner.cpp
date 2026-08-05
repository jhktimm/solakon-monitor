#include "network_scanner.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <sstream>
#include <algorithm>

namespace solakon::scanner {

bool NetworkScanner::parse_cidr(const std::string& cidr, uint32_t& base_ip, uint8_t& prefix) {
    auto slash = cidr.find('/');
    if (slash == std::string::npos) return false;

    std::string ip_str = cidr.substr(0, slash);
    std::string prefix_str = cidr.substr(slash + 1);

    struct in_addr addr{};
    if (inet_pton(AF_INET, ip_str.c_str(), &addr) != 1) return false;

    base_ip = ntohl(addr.s_addr);
    prefix = static_cast<uint8_t>(std::stoul(prefix_str));
    if (prefix > 32) return false;

    // Mask to network address
    uint32_t mask = prefix == 0 ? 0 : (~0U << (32 - prefix));
    base_ip &= mask;
    return true;
}

bool NetworkScanner::try_connect(uint32_t ip, uint16_t port, int timeout_ms) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return false;

    // Set timeout
    struct timeval tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Non-blocking for connect
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(ip);

    int ret = ::connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == 0) {
        ::close(fd);
        return true;
    }

    if (errno != EINPROGRESS) {
        ::close(fd);
        return false;
    }

    // Wait for connection to complete via select
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(fd, &writefds);

    int sel = select(fd + 1, nullptr, &writefds, nullptr, &tv);
    if (sel <= 0) {
        ::close(fd);
        return false;
    }

    // Check if connection succeeded or errored
    int err = 0;
    socklen_t err_len = sizeof(err);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &err_len);
    ::close(fd);

    return (err == 0);
}

bool NetworkScanner::identify_solakon(const std::string& ip, uint16_t port) {
    // If Modbus port 502 is open, it's very likely a Solakon ONE
    if (port == 502) return true;

    // If HTTPS port 443 is open, try to identify via banner
    if (port == 443) {
        // For now, assume HTTPS on Solakon subnet is likely a Solakon device
        // A full TLS handshake would be needed for proper identification
        return true;
    }

    return false;
}

std::vector<DiscoveredDevice> NetworkScanner::scan(
    const std::string& subnet,
    const std::vector<uint16_t>& ports,
    int timeout_ms)
{
    std::vector<DiscoveredDevice> results;
    uint32_t base_ip;
    uint8_t prefix;

    if (!parse_cidr(subnet, base_ip, prefix)) {
        return results;
    }

    uint32_t host_count = 1U << (32 - prefix);
    if (host_count > 256) host_count = 256; // Limit to 256 hosts

    for (uint32_t h = 1; h < host_count; h++) {
        uint32_t ip = base_ip | h;
        DiscoveredDevice dev{};
        dev.ip = inet_ntoa(*(struct in_addr*)&ip);
        dev.is_solakon = false;

        for (uint16_t port : ports) {
            if (try_connect(ip, port, timeout_ms)) {
                dev.ports.push_back(port);
                if (identify_solakon(dev.ip, port)) {
                    dev.is_solakon = true;
                }
            }
        }

        if (!dev.ports.empty()) {
            results.push_back(std::move(dev));
        }
    }

    // Sort by IP for consistent output
    std::sort(results.begin(), results.end(),
        [](const DiscoveredDevice& a, const DiscoveredDevice& b) {
            return a.ip < b.ip;
        });

    return results;
}

std::vector<std::string> NetworkScanner::format_results(const std::vector<DiscoveredDevice>& devices) {
    std::vector<std::string> output;
    for (auto& dev : devices) {
        std::ostringstream oss;
        oss << dev.ip;
        if (dev.is_solakon) oss << " [Solakon ONE]";
        oss << " Ports: ";
        bool first = true;
        for (auto p : dev.ports) {
            if (!first) oss << ",";
            oss << p;
            first = false;
        }
        output.push_back(oss.str());
    }
    return output;
}

} // namespace solakon::scanner
