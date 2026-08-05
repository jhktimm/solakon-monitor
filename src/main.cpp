/**
 * Solakon ONE Monitor - Terminal monitoring tool
 * Reads real-time data from Solakon ONE via Modbus TCP
 * Displays it in a btop-like terminal interface
 *
 * Usage: solakonOne [IP] [--interval N] [--once]
 *   IP:    Solakon ONE IP (default: 192.168.178.121)
 *   N:     Refresh interval in Hz (default: 1)
 *   --once: Single snapshot and exit
 */

#include "solakon_device.h"
#include "modbus_client.h"
#include "ui.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <csignal>
#include <atomic>
#include <thread>

namespace {

std::atomic<bool> g_running{true};

void signal_handler(int sig) {
    (void)sig;
    g_running = false;
}

struct Config {
    std::string host = "192.168.178.121";
    int refresh_hz = 1;
    bool once = false;
};

Config parse_args(int argc, char* argv[]) {
    Config cfg;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--once") == 0) {
            cfg.once = true;
        } else if (strcmp(argv[i], "--interval") == 0 && i + 1 < argc) {
            cfg.refresh_hz = atoi(argv[++i]);
            if (cfg.refresh_hz < 1) cfg.refresh_hz = 1;
            if (cfg.refresh_hz > 60) cfg.refresh_hz = 60;
        } else if (argv[i][0] != '-') {
            cfg.host = argv[i];
        }
    }
    return cfg;
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    Config cfg = parse_args(argc, argv);

    // Setup signal handlers
    struct sigaction sa{};
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    solakon::ui::TerminalUI ui;
    ui.init();

    solakon::SolakonDevice device;
    auto start = std::chrono::steady_clock::now();
    auto last_refresh = start;

    std::printf("Solakon ONE Monitor - Verbinde mit %s:%d ...\n",
                cfg.host.c_str(), 502);
    std::fflush(stdout);

    if (!device.connect(cfg.host)) {
        std::printf("\033[2J\033[H");
        std::printf("%s\n",
            solakon::ui::TerminalUI::bold(solakon::ui::Color::RED,
                "Fehler: Verbindung zum Solakon ONE fehlgeschlagen!")
            .c_str());
        std::printf("\n");
        std::printf("  %s\n",
            solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY,
                "Stelle sicher, dass der Solakon One im Netzwerk erreichbar ist.")
                .c_str());
        std::printf("  %s\n",
            solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY,
                "IP: " + cfg.host + " Port: 502")
                .c_str());
        std::printf("\n");
        std::printf("  %s\n",
            solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY,
                "Drücke ENTER zum Beenden")
                .c_str());
        std::getchar();
        ui.shutdown();
        return 1;
    }

    std::printf("%s\n",
        solakon::ui::TerminalUI::bold(solakon::ui::Color::GREEN,
            "Erfolgreich verbunden! Starte Monitor...")
            .c_str());
    std::printf("\n");
    std::fflush(stdout);

    while (g_running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_refresh).count();
        auto interval = 1000 / cfg.refresh_hz;

    if (cfg.once || elapsed >= interval) {
        last_refresh = now;
        auto snap = device.fetch_snapshot();
        ui.render(snap, cfg.refresh_hz);
        if (cfg.once) break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    device.disconnect();
    ui.shutdown();

    return 0;
}
