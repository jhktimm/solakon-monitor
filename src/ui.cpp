#include "ui.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <chrono>
#include <sys/ioctl.h>
#include <unistd.h>

namespace solakon::ui {

static const char* COLOR_RESET = "\033[0m";
static const char* COLOR_BOLD = "\033[1m";

static const char* fg_colors[] = {
    "\033[30m", "\033[31m", "\033[32m", "\033[33m",
    "\033[34m", "\033[35m", "\033[36m", "\033[37m",
    "\033[90m", "\033[91m", "\033[92m", "\033[93m",
    "\033[94m", "\033[95m", "\033[96m", "\033[97m",
};

static const char* bg_colors[] = {
    "\033[40m", "\033[41m", "\033[42m", "\033[43m",
    "\033[44m", "\033[45m", "\033[46m", "\033[47m",
    "\033[100m", "\033[101m", "\033[102m", "\033[103m",
    "\033[104m", "\033[105m", "\033[106m", "\033[107m",
};

std::string TerminalUI::color(Color fg, Color bg, const std::string& text) {
    std::string result;
    if (bg != Color::DEFAULT) {
        int bg_idx = static_cast<int>(bg);
        if (bg_idx >= 100 && bg_idx <= 107) {
            // BG_DARKER range: direct ANSI bg codes (100-107)
            char buf[8];
            std::snprintf(buf, sizeof(buf), "\033[%dm", bg_idx);
            result += buf;
        } else if (bg_idx >= 0 && bg_idx < 16) {
            result += bg_colors[bg_idx];
        }
    }
    int fg_idx = static_cast<int>(fg);
    if (fg_idx >= 0 && fg_idx < 16) {
        result += fg_colors[fg_idx];
    }
    result += text;
    result += COLOR_RESET;
    return result;
}

std::string TerminalUI::bold(Color fg, const std::string& text) {
    return std::string(COLOR_BOLD) + fg_colors[static_cast<int>(fg)] + text + COLOR_RESET;
}

std::string TerminalUI::dim(Color fg, const std::string& text) {
    return fg_colors[static_cast<int>(fg)] + text + COLOR_RESET;
}

std::string TerminalUI::separator(int width, char c, Color fg) {
    return color(fg, Color::BG_DARKER, std::string(static_cast<size_t>(width), c));
}

std::string TerminalUI::section_header(const std::string& title, int width, Color fg) {
    int pad = (width - static_cast<int>(title.size()) - 2) / 2;
    if (pad < 0) pad = 0;
    std::string pad_str(static_cast<size_t>(pad), ' ');
    return bold(fg, pad_str + title + " ");
}

std::string TerminalUI::status_badge(const std::string& label, bool ok, Color) {
    return ok ? bold(Color::GREEN, label) : bold(Color::RED, label);
}

std::string TerminalUI::format_power(float watts) {
    if (watts >= 1000.0f) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.2f kW", watts / 1000.0f);
        return std::string(buf);
    }
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.0f W", watts);
    return std::string(buf);
}

std::string TerminalUI::format_voltage(float v) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.1f V", v);
    return std::string(buf);
}

std::string TerminalUI::format_current(float a) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.2f A", a);
    return std::string(buf);
}

std::string TerminalUI::format_uptime(std::chrono::system_clock::time_point tp) {
    auto now = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - tp).count();
    char buf[64];
    if (diff >= 86400) {
        long long d = diff / 86400;
        long long h = (diff % 86400) / 3600;
        long long m = (diff % 3600) / 60;
        long long s = diff % 60;
        std::snprintf(buf, sizeof(buf), "%lldd %02lld:%02lld:%02lld", d, h, m, s);
    } else {
        long long h = (diff / 3600) % 24;
        long long m = (diff % 3600) / 60;
        long long s = diff % 60;
        std::snprintf(buf, sizeof(buf), "%02lld:%02lld:%02lld", h, m, s);
    }
    return std::string(buf);
}

std::string TerminalUI::bar(float value, float max_val, int width, Color fg, BarStyle) {
    if (max_val <= 0.0f) return std::string(static_cast<size_t>(width), ' ');
    float ratio = std::min(value / max_val, 1.0f);
    int filled = static_cast<int>(std::round(ratio * width));
    if (filled > width) filled = width;
    if (filled < 0) filled = 0;

    std::string result;
    result += color(fg, Color::BG_DARKER, std::string(static_cast<size_t>(filled), '#'));
    result += color(Color::DIM_GRAY, Color::BG_DARKER, std::string(static_cast<size_t>(width - filled), ' '));
    return result;
}

void TerminalUI::init() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != -1) {
        screen_width_ = w.ws_col;
        screen_height_ = w.ws_row;
    }
    if (screen_width_ < 80) screen_width_ = 80;
    if (screen_height_ < 24) screen_height_ = 24;

    std::printf("\033[2J\033[H");
    std::printf("\033[?1049h");
    std::printf("\033[?25l");
    std::fflush(stdout);
}

void TerminalUI::shutdown() {
    std::printf("\033[?1049l");
    std::printf("\033[?25h");
    std::printf("\033[0m");
    std::fflush(stdout);
}

void TerminalUI::clear() {
    std::printf("\033[2J\033[H");
    std::fflush(stdout);
}

void TerminalUI::render(const DeviceSnapshot& snap, int refresh_hz) {
    (void)refresh_hz;
    // Re-read terminal size each render in case it changed
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != -1) {
        screen_width_ = w.ws_col;
        screen_height_ = w.ws_row;
    }
    if (screen_width_ < 80) screen_width_ = 80;
    if (screen_height_ < 24) screen_height_ = 24;

    clear();
    draw_header(snap);
    draw_inverter_info(snap);
    draw_status(snap);
    draw_power(snap);
    draw_meter_data(snap);
    draw_battery(snap.bms);
    draw_energy(snap.energy);
    draw_footer();
    std::fflush(stdout);
}

void TerminalUI::draw_header(const DeviceSnapshot& snap) {
    std::string title = "Solakon ONE Monitor";
    int pad = (screen_width_ - static_cast<int>(title.size())) / 2;
    std::string pad_str(static_cast<size_t>(pad > 0 ? pad : 0), ' ');
    std::printf("%s%s\n", dim(Color::DIM_GRAY, pad_str.c_str()).c_str(),
                bold(Color::BRIGHT_CYAN, title).c_str());

    std::string info = "IP: 192.168.178.121  |  Modbus TCP :502  |  Uptime: " + format_uptime(snap.timestamp);
    std::printf("%s\n", dim(Color::DIM_GRAY, info.c_str()).c_str());

    std::printf("%s\n", separator(screen_width_, '-', Color::DIM_GRAY).c_str());
}

void TerminalUI::draw_inverter_info(const DeviceSnapshot& snap) {
    std::printf("\n%s\n", section_header("Wechselrichter", screen_width_).c_str());
    std::printf("  Modell: %s\n", bold(Color::BRIGHT_WHITE, snap.info.model_name).c_str());
    std::printf("  SN: %s\n", dim(Color::DIM_GRAY, snap.info.serial_number.c_str()).c_str());

    auto status = static_cast<InverterStatus>(snap.info.status1);
    std::string status_str;
    Color status_color;
    switch (status) {
        case InverterStatus::STANDBY:
            status_str = "Standby";
            status_color = Color::YELLOW;
            break;
        case InverterStatus::OPERATING:
            status_str = "Betrieb";
            status_color = Color::GREEN;
            break;
        case InverterStatus::ERROR:
            status_str = "Fehler!";
            status_color = Color::RED;
            break;
        default:
            status_str = "Reserviert";
            status_color = Color::DIM_GRAY;
            break;
    }
    std::printf("  Status: %s\n", bold(status_color, status_str).c_str());
    std::printf("  Rated: %.2f kW | Max: %.2f kW | Max apparent: %.2f kVA\n",
                snap.info.rated_power_kw, snap.info.max_active_kw, snap.info.max_apparent_kva);
    std::printf("%s\n", separator(screen_width_, ' ', Color::DIM_GRAY).c_str());
}

void TerminalUI::draw_status(const DeviceSnapshot& snap) {
    std::printf("\n%s\n", section_header("Systemstatus", screen_width_).c_str());
    std::printf("  %s  Verbunden: %s\n",
                status_badge("Modbus", snap.valid, Color::GREEN).c_str(),
                snap.valid ? bold(Color::GREEN, "Ja").c_str() : dim(Color::RED, "Nein").c_str());
    std::printf("  %s\n", separator(screen_width_, ' ', Color::DIM_GRAY).c_str());
}

void TerminalUI::draw_power_block(const std::string& label, float power_w, int width) {
    (void)width;
    std::printf("  %-18s %s\n", (label + ":").c_str(), format_power(power_w).c_str());
}

void TerminalUI::draw_power(const DeviceSnapshot& snap) {
    std::printf("\n%s\n", section_header("Leistungen", screen_width_).c_str());

    draw_power_block("PV Leistung", snap.energy.pv_total_power_w, screen_width_);
    draw_power_block("AC Leistung", snap.energy.ac_active_power_w, screen_width_);
    draw_power_block("Smart Meter", snap.energy.smart_meter_power_w, screen_width_);
    draw_power_block("Batterie", snap.energy.battery_power_w, screen_width_);
    
    draw_network_diagram(snap);
    
    std::printf("%s\n", separator(screen_width_, ' ', Color::DIM_GRAY).c_str());
}

void TerminalUI::draw_meter_data(const DeviceSnapshot& snap) {
    if (snap.meter1.connected) {
        std::printf("\n%s\n", section_header("Meter1/CT1", screen_width_).c_str());
        draw_meter(snap.meter1, "Meter1");
    }
    if (snap.meter2.connected) {
        std::printf("\n%s\n", section_header("Meter2/CT2", screen_width_).c_str());
        draw_meter(snap.meter2, "Meter2");
    }
}

void TerminalUI::draw_meter(const MeterData& m, const std::string& title) {
    (void)title;
    std::printf("  Spannung: %s\n", format_voltage(m.r_voltage).c_str());
    std::printf("  Strom: %s\n", format_current(m.r_current).c_str());
    std::printf("  Leistung: %s\n", format_power(m.combined_active_power).c_str());
    std::printf("  Frequenz: %.2f Hz\n", m.frequency);
    std::printf("%s\n", separator(screen_width_, ' ', Color::DIM_GRAY).c_str());
}

void TerminalUI::draw_battery(const BMSData& bms) {
    std::printf("\n%s\n", section_header("Batterie (BMS)", screen_width_).c_str());
    std::printf("  SoC: %.0f%%\n", bms.soc);
    std::string soc_bar = bar(bms.soc, 100.0f, screen_width_ - 10,
                              bms.soc > 20 ? Color::GREEN : Color::RED,
                              BarStyle::BLOCK);
    std::printf("    %s\n", soc_bar.c_str());
    std::printf("  Spannung: %.1f V | Strom: %.1f A | Temp: %.1f °C\n",
                bms.voltage, bms.current, bms.temperature);
    std::printf("  SoH: %.0f%% | Restenergie: %.1f Wh\n", bms.soh, bms.remain_energy_wh);
    std::printf("  Max Zelle: %.0f mV | Min Zelle: %.0f mV\n",
                bms.max_cell_voltage, bms.min_cell_voltage);
    std::printf("%s\n", separator(screen_width_, ' ', Color::DIM_GRAY).c_str());
}

void TerminalUI::draw_energy(const EnergyData& energy) {
    std::printf("\n%s\n", section_header("Energiebilanz", screen_width_).c_str());
    std::printf("  PV Total: %.1f kWh\n", energy.pv_total_power_w / 1000.0f);
    std::printf("  Laden (gesamt): %.1f kWh\n", energy.total_charge_kwh);
    std::printf("  Laden (heute): %.1f kWh\n", energy.total_charge_today_kwh);
    std::printf("  Entladen (gesamt): %.1f kWh\n", energy.total_discharge_kwh);
    std::printf("  Entladen (heute): %.1f kWh\n", energy.total_discharge_today_kwh);
    std::printf("  Einspeisung (gesamt): %.1f kWh\n", energy.total_feeder_kwh);
    std::printf("  Einspeisung (heute): %.1f kWh\n", energy.total_feeder_today_kwh);
    std::printf("  Verbrauch (gesamt): %.1f kWh\n", energy.total_consumption_kwh);
    std::printf("  Verbrauch (heute): %.1f kWh\n", energy.total_consumption_today_kwh);
    std::printf("  Ausgang (gesamt): %.1f kWh\n", energy.total_output_kwh);
    std::printf("  Ausgang (heute): %.1f kWh\n", energy.total_output_today_kwh);
    std::printf("  Last (gesamt): %.1f kWh\n", energy.total_load_kwh);
    std::printf("  Last (heute): %.1f kWh\n", energy.total_load_today_kwh);
    std::printf("%s\n", separator(screen_width_, ' ', Color::DIM_GRAY).c_str());
}

void TerminalUI::draw_footer() {
    std::string footer = " [ESC] Beenden  |  [R] Refresh  |  [H] Hilfe  |  [q] Quit";
    std::string pad(static_cast<size_t>(screen_width_ - static_cast<int>(footer.size()) - 1), ' ');
    std::printf("\n%s%s%s\n", dim(Color::DIM_GRAY, footer.c_str()).c_str(),
                dim(Color::DIM_GRAY, pad.c_str()).c_str(), "\033[0m");
    std::fflush(stdout);
}

void TerminalUI::draw_network_diagram(const DeviceSnapshot& snap) {
    float pv_power = snap.energy.pv_total_power_w;
    float ac_power = snap.energy.ac_active_power_w;
    float battery_power = snap.energy.battery_power_w;
    float smart_power = snap.energy.smart_meter_power_w;
    
    // UTF-8 box-drawing characters
    const char* VLINE = "\xe2\x94\x82";  // │
    const char* HLINE = "\xe2\x94\x80";  // ─
    const char* LARROW = "\xe2\x86\x90"; // ←
    const char* RARROW = "\xe2\x86\x92"; // →
    const char* SUN = "\xe2\x98\x80";    // ☀
    
    int center = screen_width_ / 2;
    
    // Layout:
    //         [Netz] -- [Haus] -- [Sakon One] --┬-- [Batterie]
    //                                               \-- [PV]
    //  netz_w  haus_w   sakon_w   split_w  batt_w  pv_w
    
    // Row 1: PV panel (centered, yellow)
    std::string pv_label = "  " + std::string(SUN) + " PV-Anlage  ";
    int pv_x = center - static_cast<int>(pv_label.size()) / 2;
    if (pv_x < 0) pv_x = 0;
    int pv_rest = screen_width_ - pv_x - static_cast<int>(pv_label.size());
    if (pv_rest < 0) pv_rest = 0;
    std::printf("%*s%s%s%s\n", pv_x, "", bold(Color::YELLOW, pv_label).c_str(),
                dim(Color::DIM_GRAY, std::string(pv_rest, ' ')).c_str(), COLOR_RESET);
    
    // PV power value with vertical line
    std::string pv_power_str = format_power(pv_power);
    int pv_power_x = center - static_cast<int>(pv_power_str.size()) / 2;
    if (pv_power_x < 0) pv_power_x = 0;
    std::printf("%*s%s%*s%s%*s\n", center, "", VLINE,
                (pv_power_x > center + 1) ? pv_power_x - center - 1 : 0, "",
                pv_power_str.c_str(),
                screen_width_ - pv_power_x - static_cast<int>(pv_power_str.size()) > 0
                    ? screen_width_ - pv_power_x - static_cast<int>(pv_power_str.size()) : 0, "");
    
    // PV connection line to split row
    std::printf("%*s%s%*s\n", center, "", VLINE, screen_width_ - center - 1, "");
    
    // Row 2: split point (┬) connecting Sakon One to Batterie and PV
    int split_x = center;
    std::string row2 = std::string(split_x, ' ');
    row2 += "\xe2\x94\xb4";  // ┴
    row2 += std::string(screen_width_ - split_x - 1, ' ');
    std::printf("%s\n", row2.c_str());
    
    // Row 3: PV connection to split
    int pv_w = 10;  // [PV] visual width
    int pv_row_x = center - pv_w / 2;
    if (pv_row_x < 2) pv_row_x = 2;
    std::string row3 = std::string(pv_row_x, ' ');
    row3 += bold(Color::YELLOW, " [PV]   ");
    int pv_rest3 = screen_width_ - pv_row_x - static_cast<int>(row3.size());
    if (pv_rest3 > 0) row3 += std::string(pv_rest3, ' ');
    std::printf("%s\n", row3.c_str());
    
    // PV value below
    std::string pv_val_str = format_power(pv_power);
    int pv_val_x = pv_row_x + pv_w / 2;
    std::printf("%*s%s%*s\n", pv_val_x, "",
                dim(Color::DIM_GRAY, pv_val_str).c_str(),
                screen_width_ - pv_val_x - static_cast<int>(pv_val_str.size()) > 0
                    ? screen_width_ - pv_val_x - static_cast<int>(pv_val_str.size()) : 0, "");
    
    // Row 4: [Netz] -- [Haus] -- [Sakon One]
    int netz_w = 14;  // [Öff. Netz] visual width
    int haus_w = 12;  // [Haus] visual width
    int sakon_w = 14; // [Sakon One] visual width
    int gap = 4;
    
    int total_w = netz_w + haus_w + sakon_w + gap * 2;
    int row_start = center - total_w / 2;
    if (row_start < 2) row_start = 2;
    
    int netz_x = row_start;
    int haus_x = netz_x + netz_w + gap;
    int sakon_x = haus_x + haus_w + gap;
    
    // Track visual cursor
    int cursor = row_start;
    
    // Padding
    if (cursor > 0) std::printf("%*s", cursor, "");
    cursor = 0;
    
    // Netz box (cyan)
    std::string netz_box = "  [Öff. Netz]  ";
    std::printf("%s", bold(Color::CYAN, netz_box).c_str());
    cursor += static_cast<int>(netz_box.size());
    
    // Line to Haus
    int line1_len = haus_x - cursor - 1;
    if (line1_len < 0) line1_len = 0;
    if (line1_len > 0) {
        for (int i = 0; i < line1_len; i++) std::printf("%s", HLINE);
    }
    cursor += line1_len;
    
    // Arrow between Netz and Haus
    // smart_power > 0 = feed-in (Haus -> Netz)
    // smart_power < 0 = consumption (Netz -> Haus)
    if (smart_power > 0) {
        std::printf("%s", LARROW);  // Haus -> Netz
    } else if (smart_power < 0) {
        std::printf("%s", RARROW);  // Netz -> Haus
    } else {
        std::printf("-");
    }
    cursor++;
    
    // Continue line to Haus
    int line1_rest = haus_x - cursor - 1;
    if (line1_rest < 0) line1_rest = 0;
    if (line1_rest > 0) {
        for (int i = 0; i < line1_rest; i++) std::printf("%s", HLINE);
    }
    cursor += line1_rest;
    
    // Haus box (green)
    std::string haus_box = "    [Haus]    ";
    std::printf("%s", bold(Color::GREEN, haus_box).c_str());
    cursor += static_cast<int>(haus_box.size());
    
    // Line to Sakon
    int line2_len = sakon_x - cursor - 1;
    if (line2_len < 0) line2_len = 0;
    if (line2_len > 0) {
        for (int i = 0; i < line2_len; i++) std::printf("%s", HLINE);
    }
    cursor += line2_len;
    
    // Arrow between Haus and Sakon
    // ac_power > 0 = output (Haus -> Sakon)
    // ac_power < 0 = input (Sakon -> Haus)
    if (ac_power > 0) {
        std::printf("%s", RARROW);  // Haus -> Sakon
    } else if (ac_power < 0) {
        std::printf("%s", LARROW);  // Haus <- Sakon
    } else {
        std::printf("-");
    }
    cursor++;
    
    // Continue line to Sakon
    int line2_rest = sakon_x - cursor - 1;
    if (line2_rest < 0) line2_rest = 0;
    if (line2_rest > 0) {
        for (int i = 0; i < line2_rest; i++) std::printf("%s", HLINE);
    }
    cursor += line2_rest;
    
    // Sakon One box (white/bright)
    std::string sakon_box = " [Sakon One]  ";
    std::printf("%s", bold(Color::BRIGHT_WHITE, sakon_box).c_str());
    cursor += static_cast<int>(sakon_box.size());
    
    // Fill rest
    int rest = screen_width_ - cursor;
    if (rest > 0) std::printf("%*s", rest, "");
    std::printf("\n");
    
    // Power values below each box
    int netz_val_x = netz_x + netz_w / 2;
    int haus_val_x = haus_x + haus_w / 2;
    int sakon_val_x = sakon_x + sakon_w / 2;
    
    std::string netz_val = format_power(smart_power);
    std::string haus_val = format_power(ac_power);
    std::string sakon_val = format_power(battery_power);
    
    // Pad to netz_val_x
    if (netz_x > 0) std::printf("%*s", netz_x, "");
    
    // Netz value centered
    int netz_pad = netz_val_x - static_cast<int>(netz_val.size()) / 2 - netz_x;
    if (netz_pad < 0) netz_pad = 0;
    if (netz_pad > 0) std::printf("%*s", netz_pad, "");
    std::printf("%s", dim(Color::DIM_GRAY, netz_val).c_str());
    
    // Haus value
    int haus_pad = haus_val_x - static_cast<int>(netz_val.size()) - netz_x - netz_pad;
    if (haus_pad < 0) haus_pad = 0;
    if (haus_pad > 0) std::printf("%*s", haus_pad, "");
    std::printf("%s", dim(Color::DIM_GRAY, haus_val).c_str());
    
    // Sakon value
    int sakon_pad = sakon_val_x - static_cast<int>(netz_val.size()) - static_cast<int>(haus_val.size()) - netz_x - netz_pad - haus_pad;
    if (sakon_pad < 0) sakon_pad = 0;
    if (sakon_pad > 0) std::printf("%*s", sakon_pad, "");
    std::printf("%s", dim(Color::DIM_GRAY, sakon_val).c_str());
    
    int rest5 = screen_width_ - netz_x - netz_pad - static_cast<int>(netz_val.size()) - haus_pad - static_cast<int>(haus_val.size()) - sakon_pad - static_cast<int>(sakon_val.size());
    if (rest5 > 0) std::printf("%*s", rest5, "");
    std::printf("\n");
    
    // Row 5: vertical line from Sakon to split
    int sakon_center = sakon_x + sakon_w / 2;
    if (sakon_center >= 0 && sakon_center < screen_width_) {
        std::string row5 = std::string(sakon_center, ' ');
        row5 += VLINE;
        row5 += std::string(screen_width_ - sakon_center - 1, ' ');
        std::printf("%s\n", row5.c_str());
    }
}

} // namespace solakon::ui