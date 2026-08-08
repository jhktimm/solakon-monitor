#pragma once
/**
 * Terminal UI for Solakon ONE monitoring
 * btop-like design with colors, borders, and real-time updates
 */

#include "solakon_device.h"

#include <string>
#include <chrono>

namespace solakon::ui {

enum class Color {
    BLACK = 0, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE,
    DIM_GRAY = 8, BRIGHT_RED, BRIGHT_GREEN, BRIGHT_YELLOW,
    BRIGHT_BLUE, BRIGHT_MAGENTA, BRIGHT_CYAN, BRIGHT_WHITE,
    BG_DARKER = 100, DEFAULT = -1,
};

enum class BarStyle { BLOCK, LINE };

class TerminalUI {
public:
    TerminalUI() = default;
    ~TerminalUI() = default;
    TerminalUI(const TerminalUI&) = delete;
    TerminalUI& operator=(const TerminalUI&) = delete;

    void init();
    void shutdown();
    // void clear();  // disabled — clearing screen during render caused visual artifacts (clear-after-render bug)
    void render(const DeviceSnapshot& snap, int refresh_hz);
    void render_json(const DeviceSnapshot& snap);
    void render_art(const DeviceSnapshot& snap, int refresh_hz);

public:
    // Formatting helpers - public for use in main.cpp
    static std::string color(Color fg, Color bg = Color::DEFAULT, const std::string& text = "");
    static std::string bold(Color fg, const std::string& text = "");
    static std::string dim(Color fg, const std::string& text = "");
    static std::string separator(int width, char c = '-', Color fg = Color::DIM_GRAY);
    static std::string section_header(const std::string& title, int width, Color fg = Color::BRIGHT_CYAN);
    static std::string status_badge(const std::string& label, bool ok, Color fg = Color::GREEN);
    static std::string format_power(float watts);
    static std::string format_voltage(float v);
    static std::string format_current(float a);
    static std::string format_uptime(std::chrono::system_clock::time_point tp);
    static std::string bar(float value, float max_val, int width, Color fg = Color::GREEN, BarStyle style = BarStyle::BLOCK);
    void draw_power_block(const std::string& label, float power_w, int width);

private:
    int screen_width_ = 80;
    int screen_height_ = 24;

    void draw_header(const DeviceSnapshot& snap);
    void draw_inverter_info(const DeviceSnapshot& snap);
    void draw_status(const DeviceSnapshot& snap);
    void draw_power(const DeviceSnapshot& snap);
    void draw_meter_data(const DeviceSnapshot& snap);
    void draw_meter(const MeterData& m, const std::string& title);
    void draw_battery(const BMSData& bms);
    void draw_energy(const EnergyData& energy);
    void draw_footer();
    void draw_network_diagram(const DeviceSnapshot& snap);
};

} // namespace solakon::ui
