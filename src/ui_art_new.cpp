void TerminalUI::render_art(const DeviceSnapshot& snap, int refresh_hz) {
    (void)refresh_hz;
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != -1 && w.ws_col > 0) {
        screen_width_ = w.ws_col;
    }
    if (screen_width_ < 80) screen_width_ = 80;

    // Move cursor to top-left and hide it
    std::printf("\033[H\033[?25l");
    std::fflush(stdout);

    float pv   = snap.energy.pv_total_power_w;
    float smart_m = snap.energy.smart_meter_power_w;
    float batt = snap.energy.battery_power_w;
    float soc  = snap.bms.soc;

    // Helper lambda: clear line, print with optional center padding
    auto print_line = [&](int pad, const std::string& text) {
        std::printf("\033[2K%*s%s\n", pad, "", text.c_str());
    };

    // Title bar
    print_line(0, "\033[36mSolakon ONE Monitor - Art Mode\033[0m");

    // Line 1: PV power flow toward inverter
    std::string pv_line = (pv > 100)
        ? "PV -> +" + format_power(pv) + " ->"
        : "PV --- OFF";
    int pad1 = (screen_width_ - static_cast<int>(pv_line.size())) / 2;
    if (pad1 < 0) pad1 = 0;
    print_line(pad1, pv_line);

    // Line 2: Smart Meter / Grid flow
    std::string sm_line;
    if (smart_m > 100)
        sm_line = "NETZ -> +" + format_power(smart_m) + " -> SM";
    else if (smart_m < -100)
        sm_line = "SM -> +" + format_power(-smart_m) + " -> NETZ";
    else
        sm_line = "NETZ --- 0W ---";
    int pad2 = (screen_width_ - static_cast<int>(sm_line.size())) / 2;
    if (pad2 < 0) pad2 = 0;
    print_line(pad2, sm_line);

    // Line 3: Battery status
    std::string batt_line = (batt > 5)
        ? "BATT [Lade +" + format_power(batt) + "] SOC:" + std::to_string((int)soc) + "%]"
        : batt < -5
            ? "BATT [Entl +-" + format_power(-batt) + "] SOC:" + std::to_string((int)soc) + "%]"
            : "BATT [SOC:" + std::to_string((int)soc) + "%]";
    int pad3 = (screen_width_ - static_cast<int>(batt_line.size())) / 2;
    if (pad3 < 0) pad3 = 0;
    print_line(pad3, batt_line);

    // Footer
    std::printf("\033[K\033[90m%*s\033[0m\n", screen_width_ - 16, "  [q] Quit");
    std::fflush(stdout);
}
