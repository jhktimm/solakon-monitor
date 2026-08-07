/**
 * Solakon ONE Monitor - Terminal monitoring tool
 * Reads real-time data from Solakon ONE via Modbus TCP
 * Displays it in a btop-like terminal interface
 *
 * Usage: solakon-monitor [OPTIONS] [IP]
 *   IP:         Solakon ONE IP (from config, CLI, or scan)
 *   --config X  Use config file X instead of default
 *   --scan      Scan network for Solakon ONE devices and exit
 *   --once      Single snapshot and exit
 *   --json      Output single snapshot as JSON
 *   --server    Start HTTP server (default port: 8080)
 *   --interval N Refresh interval in Hz (1-60, default: 1)
 *   --art       ASCII art network diagram mode
 *   -p, --port PORT Solakon ONE Modbus port (default: 502)
 *   -h, --help  Show this help message
 */

#include "solakon_device.h"
#include "modbus_client.h"
#include "ui.h"
#include "http_server.h"
#include "config_loader.h"
#include "network_scanner.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <csignal>
#include <atomic>
#include <thread>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <sys/select.h>

namespace {

std::atomic<bool> g_running{true};

void signal_handler(int sig) {
    (void)sig;
    g_running = false;
}

struct TermiosSaver {
    struct termios orig_termios;
    bool saved = false;

    void save() {
        if (tcgetattr(STDIN_FILENO, &orig_termios) == 0) {
            saved = true;
        }
    }

    void raw() {
        if (!saved) return;
        struct termios raw = orig_termios;
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    }

    ~TermiosSaver() {
        if (saved) {
            tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
        }
    }
} termios_saver;

struct Config {
    std::string host = "";
    int refresh_hz = 1;
    uint16_t modbus_port = 502;
    bool once = false;
    bool json = false;
    bool server = false;
    int server_port = 8080;
    bool help = false;
    bool scan = false;
    bool art = false;
    std::string config_path = "";
};

void print_usage(const char* prog) {
    std::printf("Usage: %s [OPTIONS] [IP]\n", prog);
    std::printf("\n");
    std::printf("Real-time monitoring for Solakon ONE hybrid solar inverters via Modbus TCP.\n");
    std::printf("\n");
    std::printf("Options:\n");
    std::printf("  -h, --help          Show this help message\n");
    std::printf("  --config FILE       Use config file FILE\n");
    std::printf("  --scan              Scan network for Solakon ONE devices\n");
    std::printf("  --once              Single snapshot and exit\n");
    std::printf("  --json              Output single snapshot as JSON\n");
    std::printf("  --server [PORT]     Start HTTP server (default port: 8080)\n");
    std::printf("  --interval N        Refresh interval in Hz (1-60, default: 1)\n");
    std::printf("  --art               ASCII art network diagram mode\n");
    std::printf("  -p, --port PORT     Solakon ONE Modbus port (default: 502)\n");
    std::printf("\n");
    std::printf("Config file (auto-discovered in this order):\n");
    std::printf("  1. --config <path>\n");
    std::printf("  2. ~/.config/solakon-monitor/solakon.conf\n");
    std::printf("  3. ./solakon.conf\n");
    std::printf("\n");
    std::printf("Examples:\n");
    std::printf("  %s                          # Terminal monitor (from config)\n", prog);
    std::printf("  %s --scan                   # Scan for devices\n", prog);
    std::printf("  %s 192.168.178.121         # With custom IP\n", prog);
    std::printf("  %s --once                   # Single snapshot\n", prog);
    std::printf("  %s --once --json            # JSON output\n", prog);
    std::printf("  %s --server                 # HTTP server mode\n", prog);
    std::printf("  %s --server 9090            # HTTP server on port 9090\n", prog);
    std::printf("\n");
    std::printf("HTTP Server endpoints (when --server is used):\n");
    std::printf("  GET /data   Full snapshot as JSON\n");
    std::printf("  GET /health Health check\n");
}

Config parse_args(int argc, char* argv[]) {
    Config cfg;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            cfg.help = true;
        } else if (strcmp(argv[i], "--config") == 0) {
            if (i + 1 < argc) {
                cfg.config_path = argv[++i];
            }
        } else if (strcmp(argv[i], "--scan") == 0) {
            cfg.scan = true;
        } else if (strcmp(argv[i], "--port") == 0 || strcmp(argv[i], "-p") == 0) {
            if (i + 1 < argc) {
                cfg.modbus_port = atoi(argv[++i]);
                if (cfg.modbus_port < 1) {
                    cfg.modbus_port = 502;
                }
            }
        } else if (strcmp(argv[i], "--once") == 0) {
            cfg.once = true;
        } else if (strcmp(argv[i], "--json") == 0) {
            cfg.json = true;
        } else if (strcmp(argv[i], "--server") == 0) {
            cfg.server = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                cfg.server_port = atoi(argv[++i]);
                if (cfg.server_port < 1 || cfg.server_port > 65535) {
                    cfg.server_port = 8080;
                }
            }
        } else if (strcmp(argv[i], "--interval") == 0 && i + 1 < argc) {
            cfg.refresh_hz = atoi(argv[++i]);
            if (cfg.refresh_hz < 1) cfg.refresh_hz = 1;
            if (cfg.refresh_hz > 60) cfg.refresh_hz = 60;
        } else if (strcmp(argv[i], "--art") == 0) {
            cfg.art = true;
        } else if (argv[i][0] != '-') {
            cfg.host = argv[i];
        }
    }
    return cfg;
}

// Interactive prompt for IP address
std::string prompt_ip(const char* prompt) {
    std::printf("%s\n", prompt);
    std::printf("IP: ");
    std::fflush(stdout);
    char buf[64];
    if (fgets(buf, sizeof(buf), stdin) != nullptr) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
        if (strlen(buf) > 0) return buf;
    }
    return "";
}

// Save discovered IP to config
void save_discovered(const std::string& ip, uint16_t port, const std::string& config_path) {
    solakon::config::Settings cfg;
    cfg.host = ip;
    cfg.modbus_port = port;
    if (solakon::config::save(cfg)) {
        std::printf("\n  %s %s\n",
            solakon::ui::TerminalUI::bold(solakon::ui::Color::GREEN, "Konfiguration gespeichert:").c_str(),
            config_path.c_str());
    } else {
        std::printf("\n  %s: %s\n",
            solakon::ui::TerminalUI::bold(solakon::ui::Color::YELLOW, "Warnung: Config konnte nicht gespeichert werden").c_str(),
            config_path.c_str());
    }
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    Config cfg = parse_args(argc, argv);

    if (cfg.help) {
        print_usage(argv[0]);
        return 0;
    }

    // Scan mode
    if (cfg.scan) {
        // Load config for subnet/ports, or use defaults
        auto settings = solakon::config::load(cfg.config_path);
        if (settings.scan_ports.empty()) {
            settings.scan_ports = {502, 443};
        }

        std::printf("\n  %s %s\n\n",
            solakon::ui::TerminalUI::bold(solakon::ui::Color::CYAN, "Netzwerk-Scan:").c_str(),
            solakon::ui::TerminalUI::bold(solakon::ui::Color::BRIGHT_WHITE, settings.scan_subnet.c_str())
            .c_str());

        solakon::scanner::NetworkScanner scanner;
        auto devices = scanner.scan(settings.scan_subnet, settings.scan_ports, 2000);

        if (devices.empty()) {
            std::printf("  %s\n",
                solakon::ui::TerminalUI::bold(solakon::ui::Color::YELLOW, "Keine Geräte gefunden.")
                .c_str());
            std::printf("  %s\n",
                solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY,
                    "Pruefe: Subnet, Ports, Firewall").c_str());
            return 1;
        }

        std::printf("  %s %s\n\n",
            solakon::ui::TerminalUI::bold(solakon::ui::Color::GREEN, "Gefundene Gerichte:").c_str(),
            solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY,
                ("(" + std::to_string(devices.size()) + " gefunden)").c_str())
            .c_str());

        auto results = solakon::scanner::NetworkScanner::format_results(devices);
        for (auto& r : results) {
            std::printf("  %s\n", r.c_str());
        }

        // If exactly one Solakon device found, offer to save
        auto solakon_devs = std::count_if(devices.begin(), devices.end(),
            [](const solakon::scanner::DiscoveredDevice& d) { return d.is_solakon; });

        if (solakon_devs == 1) {
            auto solakon_ip = std::find_if(devices.begin(), devices.end(),
                [](const solakon::scanner::DiscoveredDevice& d) { return d.is_solakon; })->ip;
            auto solakon_port = devices[0].ports[0];

            std::printf("\n  %s: %s\n",
                solakon::ui::TerminalUI::bold(solakon::ui::Color::GREEN, "Vorschlag").c_str(),
                solakon::ui::TerminalUI::bold(solakon::ui::Color::BRIGHT_WHITE, solakon_ip.c_str())
                .c_str());
            std::printf("  %s\n",
                solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY,
                    "Druecke ENTER zum Speichern, oder STRG+C zum Abbrechen")
                .c_str());

            std::getchar();
            save_discovered(solakon_ip, solakon_port, settings.config_path);
        }

        return 0;
    }

    // Load config
    auto settings = solakon::config::load(cfg.config_path);

    // CLI overrides
    if (!cfg.host.empty()) {
        settings.host = cfg.host;
    }
    if (cfg.modbus_port != 502) {
        settings.modbus_port = cfg.modbus_port;
    }
    if (cfg.refresh_hz != 1) {
        settings.refresh_hz = cfg.refresh_hz;
    }

    // If no host from config or CLI, prompt user
    if (settings.host.empty()) {
        // Check if default config exists
        auto check = solakon::config::load("");
        if (check.config_path.empty()) {
            std::printf("\n  %s\n",
                solakon::ui::TerminalUI::bold(solakon::ui::Color::YELLOW,
                    "Keine Solakon ONE IP konfiguriert.")
                .c_str());
            std::printf("  %s\n",
                solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY,
                    "Verwende --scan um Guereite zu finden")
                .c_str());
            std::printf("  %s\n",
                solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY,
                    "Oder erstelle ~/.config/solakon-monitor/solakon.conf")
                .c_str());
            std::printf("  %s\n",
                solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY,
                    "Siehe README.md fuer Details")
                .c_str());
            return 1;
        }

        if (check.host.empty()) {
            std::printf("\n  %s\n",
                solakon::ui::TerminalUI::bold(solakon::ui::Color::YELLOW,
                    "Keine Solakon ONE IP konfiguriert.").c_str());
            std::printf("  %s\n",
                solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY,
                    "Verwende --scan um Gerichte zu finden")
                .c_str());
            std::printf("\n  %s\n",
                solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY,
                    "Oder manuell IP eingeben:")
                .c_str());

            std::getchar();
            auto ip = prompt_ip("Solakon ONE IP");
            if (ip.empty()) {
                std::printf("\n  %s\n",
                    solakon::ui::TerminalUI::bold(solakon::ui::Color::RED,
                        "Keine IP eingegeben. Abbruch.")
                    .c_str());
                return 1;
            }

            settings.host = ip;
            settings.modbus_port = cfg.modbus_port;
            settings.config_path = solakon::config::default_path();
            solakon::config::save(settings);
            std::printf("\n  %s: %s\n",
                solakon::ui::TerminalUI::bold(solakon::ui::Color::GREEN, "Gespeichert").c_str(),
                solakon::ui::TerminalUI::bold(solakon::ui::Color::BRIGHT_WHITE,
                    settings.config_path.c_str())
                .c_str());
        } else {
            std::printf("\n  %s\n",
                solakon::ui::TerminalUI::bold(solakon::ui::Color::YELLOW,
                    "Keine Solakon ONE IP konfiguriert.")
                .c_str());
            std::printf("  %s\n",
                solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY,
                    "Verwende --scan um Gerichte zu finden")
                .c_str());
            std::printf("  %s\n",
                solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY,
                    "Oder erstelle ~/.config/solakon-monitor/solakon.conf")
                .c_str());
            std::printf("  %s\n",
                solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY,
                    "Siehe README.md fuer Details")
                .c_str());
            return 1;
        }
    }

    // Setup signal handlers
    struct sigaction sa{};
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    solakon::SolakonDevice device;
    auto start = std::chrono::steady_clock::now();
    auto last_refresh = start;

    std::printf("Solakon ONE Monitor - Verbinde mit %s:%d ... \n",
                settings.host.c_str(), settings.modbus_port);
    std::fflush(stdout);

    if (!device.connect(settings.host, settings.modbus_port)) {
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
                "IP: " + settings.host + " Port: " + std::to_string(settings.modbus_port))
                .c_str());
        std::printf("\n");
        std::printf("  %s\n",
            solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY,
                "Druecke ENTER zum Beenden")
                .c_str());
        std::getchar();
        return 1;
    }

    // Terminal mode
    if (!cfg.server) {
        solakon::ui::TerminalUI ui;
        ui.init();

        // Save and set terminal to raw mode for non-blocking key input
        termios_saver.save();
        termios_saver.raw();

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
                if (cfg.json) {
                    ui.render_json(snap);
                } else if (cfg.art) {
                    ui.render_art(snap, cfg.refresh_hz);
                } else {
                    ui.render(snap, cfg.refresh_hz);
                }
                if (cfg.once) break;
            }

            // Non-blocking key input
            struct pollfd pfd;
            pfd.fd = STDIN_FILENO;
            pfd.events = POLLIN;
            pfd.revents = 0;
            int ret = poll(&pfd, 1, 50);
            if (ret > 0 && (pfd.revents & POLLIN)) {
                char key = 0;
                if (read(STDIN_FILENO, &key, 1) > 0) {
                    if (key == 'q' || key == 'Q') {
                        g_running = false;
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        device.disconnect();
        ui.shutdown();
    }
    // Server mode
    else {
        solakon::http::Server server(cfg.server_port);
        if (!server.start()) {
            std::printf("\033[2J\033[H");
            std::printf("%s\n",
                solakon::ui::TerminalUI::bold(solakon::ui::Color::RED,
                    "Fehler: HTTP-Server konnte nicht gestartet werden!")
                .c_str());
            std::printf("\n");
            std::printf("  %s\n",
                solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY,
                    "Port bereits belegt oder keine Berechtigung.")
                    .c_str());
            std::printf("  %s\n",
                solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY,
                    "Versuche einen anderen Port: solakon-monitor --server 9090")
                    .c_str());
            std::printf("\n");
            std::printf("  %s\n",
                solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY,
                    "Druecke ENTER zum Beenden")
                    .c_str());
            std::getchar();
            device.disconnect();
            return 1;
        }

        std::printf("%s\n",
            solakon::ui::TerminalUI::bold(solakon::ui::Color::GREEN,
                "Erfolgreich verbunden! HTTP-Server gestartet.")
            .c_str());
        std::printf("\n");
        std::printf("  %s %s\n",
            solakon::ui::TerminalUI::bold(solakon::ui::Color::CYAN, "HTTP-Server:").c_str(),
            solakon::ui::TerminalUI::bold(solakon::ui::Color::BRIGHT_WHITE,
                ("http://localhost:" + std::to_string(cfg.server_port)).c_str())
            .c_str());
        std::printf("  %s\n",
            solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY,
                "Druecke q zum Beenden")
                .c_str());
        std::printf("\n");
        std::fflush(stdout);

        while (g_running) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_refresh).count();
            auto interval = 1000 / cfg.refresh_hz;

            if (elapsed >= interval) {
                last_refresh = now;
                auto snap = device.fetch_snapshot();
                server.update_snapshot(snap);

                // Print status line every 10 updates
                static int counter = 0;
                if (++counter % 10 == 0) {
                    std::printf("\r  PV: %s | AC: %s | Batterie: %s | Smart Meter: %s    ",
                        solakon::ui::TerminalUI::format_power(snap.energy.pv_total_power_w).c_str(),
                        solakon::ui::TerminalUI::format_power(snap.energy.ac_active_power_w).c_str(),
                        solakon::ui::TerminalUI::format_power(snap.energy.battery_power_w).c_str(),
                        solakon::ui::TerminalUI::format_power(snap.energy.smart_meter_power_w).c_str());
                    std::fflush(stdout);
                }
            }

            // Non-blocking key input
            struct pollfd pfd;
            pfd.fd = STDIN_FILENO;
            pfd.events = POLLIN;
            pfd.revents = 0;
            int ret = poll(&pfd, 1, 50);
            if (ret > 0 && (pfd.revents & POLLIN)) {
                char key = 0;
                if (read(STDIN_FILENO, &key, 1) > 0) {
                    if (key == 'q' || key == 'Q') {
                        g_running = false;
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        std::printf("\n  %s\n",
            solakon::ui::TerminalUI::dim(solakon::ui::Color::DIM_GRAY,
                "Server wird gestoppt...")
                .c_str());
        server.stop();
        device.disconnect();
    }

    return 0;
}
