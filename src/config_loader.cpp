#include "config_loader.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <cstring>

namespace solakon::config {

namespace {

// Trim whitespace from both ends
std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// Check if path exists
bool path_exists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

} // anonymous namespace

Settings load(const std::string& override_path) {
    Settings cfg;
    std::string path;

    // Priority: override > ~/.config > current dir
    if (!override_path.empty()) {
        path = override_path;
    } else {
        const char* home = getenv("HOME");
        if (home) {
            path = std::string(home) + "/.config/solakon-monitor/solakon.conf";
        }
        if (!path_exists(path)) {
            path = "solakon.conf";
        }
    }

    if (!path_exists(path)) {
        cfg.config_path = path;
        return cfg;
    }

    cfg.config_path = path;
    std::ifstream file(path);
    if (!file.is_open()) {
        return cfg;
    }

    std::string line;
    std::string current_section;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;

        if (line[0] == '[' && line.back() == ']') {
            current_section = line.substr(1, line.size() - 2);
            continue;
        }

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));

        // Only parse [solakon] section
        if (current_section != "solakon") continue;

        if (key == "ip") {
            cfg.host = val;
        } else if (key == "modbus_port") {
            cfg.modbus_port = static_cast<uint16_t>(std::stoi(val));
        } else if (key == "https_port") {
            cfg.https_port = static_cast<uint16_t>(std::stoi(val));
        } else if (key == "refresh_hz") {
            cfg.refresh_hz = std::stoi(val);
        } else if (key == "scan_subnet") {
            cfg.scan_subnet = val;
        } else if (key == "scan_ports") {
            cfg.scan_ports.clear();
            std::istringstream ss(val);
            std::string port;
            while (std::getline(ss, port, ',')) {
                port = trim(port);
                if (!port.empty()) {
                    cfg.scan_ports.push_back(static_cast<uint16_t>(std::stoi(port)));
                }
            }
        }
    }

    return cfg;
}

bool save(const Settings& cfg) {
    if (cfg.config_path.empty()) return false;

    // Ensure directory exists
    auto slash_pos = cfg.config_path.find_last_of('/');
    if (slash_pos != std::string::npos) {
        std::string dir = cfg.config_path.substr(0, slash_pos);
        mkdir(dir.c_str(), 0755);
    }

    std::ofstream file(cfg.config_path, std::ios::trunc);
    if (!file.is_open()) return false;

    file << "[solakon]\n";
    file << "; IP-Adresse des Solakon ONE Wechselrichters\n";
    file << "ip = " << cfg.host << "\n\n";
    file << "; Modbus TCP Port (Standard: 502)\n";
    file << "modbus_port = " << cfg.modbus_port << "\n\n";
    file << "; HTTPS Port für Web-Oberfläche (Standard: 443)\n";
    file << "https_port = " << cfg.https_port << "\n\n";
    file << "; Aktualisierungsrate in Hz (1-60)\n";
    file << "refresh_hz = " << cfg.refresh_hz << "\n\n";
    file << "; Netzwerk-Scan-Einstellungen\n";
    file << "; Subnet zum Scannen (CIDR-Notation)\n";
    file << "scan_subnet = " << cfg.scan_subnet << "\n\n";
    file << "; Zu scannende Ports (kommagetrennt)\n";
    file << "scan_ports = ";
    for (size_t i = 0; i < cfg.scan_ports.size(); i++) {
        if (i > 0) file << ",";
        file << cfg.scan_ports[i];
    }
    file << "\n";

    return true;
}

std::string default_path() {
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/.config/solakon-monitor/solakon.conf";
    }
    return "solakon.conf";
}

} // namespace solakon::config
