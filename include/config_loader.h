#pragma once
/**
 * @file config_loader.h
 * @brief Lightweight INI config loader for solakon-monitor.
 *
 * Minimal INI parser — no external dependencies. Supports:
 *   [section]
 *   key = value
 *   ; comment
 *
 * Config file locations (in priority order):
 *   1. Path from --config <path>
 *   2. ~/.config/solakon-monitor/solakon.conf
 *   3. ./solakon.conf (current directory)
 */

#include <cstdint>
#include <string>
#include <map>
#include <optional>
#include <vector>

namespace solakon::config {

/**
 * @brief Parsed configuration values.
 */
struct Settings {
    std::string host = "";       // empty = unknown
    uint16_t modbus_port = 502;
    uint16_t https_port = 443;
    int refresh_hz = 1;
    std::string scan_subnet = "192.168.178.0/24";
    std::vector<uint16_t> scan_ports = {502, 443};
    std::string config_path = "";  // where it was loaded from
};

/**
 * @brief Load configuration from the default or specified path.
 * @param override_path Optional explicit path (from --config flag).
 * @return Settings with loaded values, or defaults if no file found.
 */
Settings load(const std::string& override_path = "");

/**
 * @brief Save settings back to the loaded config file.
 * @param settings Settings to save.
 * @return true if saved successfully.
 */
bool save(const Settings& settings);

/**
 * @brief Get the default config file path.
 * @return Path to ~/.config/solakon-monitor/solakon.conf
 */
std::string default_path();

} // namespace solakon::config
