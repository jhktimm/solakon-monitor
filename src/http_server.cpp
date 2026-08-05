#include "http_server.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <sstream>
#include <mutex>
#include <thread>

// POSIX sockets
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

namespace solakon::http {

static const char* HTTP_OK = "HTTP/1.1 200 OK\r\n";
static const char* HTTP_NOT_FOUND = "HTTP/1.1 404 Not Found\r\n";
static const char* CONTENT_TYPE_JSON = "Content-Type: application/json\r\n";
static const char* CONNECTION_CLOSE = "Connection: close\r\n";
static const char* CRLF = "\r\n";

Server::Server(int port) : port_(port) {
    // Ignore SIGPIPE to prevent crashes when client disconnects
    signal(SIGPIPE, SIG_IGN);
}

Server::~Server() {
    stop();
}

bool Server::start() {
    if (running_) return false;

    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::fprintf(stderr, "HTTP server: socket() failed\n");
        return false;
    }

    // Set SO_REUSEADDR to allow quick restart
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::fprintf(stderr, "HTTP server: setsockopt() failed\n");
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::fprintf(stderr, "HTTP server: bind() failed on port %d\n", port_);
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    if (listen(server_fd_, 16) < 0) {
        std::fprintf(stderr, "HTTP server: listen() failed\n");
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    running_ = true;
    server_thread_ = std::thread(&Server::listen_loop, this);
    return true;
}

void Server::stop() {
    if (!running_) return;
    running_ = false;

    // Close server socket to unblock accept()
    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
    }

    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

void Server::update_snapshot(const DeviceSnapshot& snap) {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    latest_snapshot_ = snap;
}

DeviceSnapshot Server::get_snapshot() const {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    return latest_snapshot_;
}

bool Server::is_running() const {
    return running_;
}

void Server::listen_loop() {
    while (running_) {
        struct sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (!running_) break;
            continue;
        }

        // Set timeout on client socket
        struct timeval tv{};
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        handle_client(client_fd);
    }
}

void Server::handle_client(int client_fd) {
    char buffer[4096];
    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    if (n <= 0) {
        close(client_fd);
        return;
    }
    buffer[n] = '\0';

    // Parse HTTP request
    std::string request(buffer, n);
    std::string path;

    // Extract path from "GET /path HTTP/1.1"
    size_t method_end = request.find(' ');
    if (method_end != std::string::npos) {
        size_t path_end = request.find(' ', method_end + 1);
        if (path_end != std::string::npos) {
            path = request.substr(method_end + 1, path_end - method_end - 1);
        }
    }

    std::string body;
    std::string status;

    if (path == "/data" || path == "/") {
        DeviceSnapshot snap = get_snapshot();
        body = snapshot_to_json(snap);
        status = HTTP_OK;
    } else if (path == "/health") {
        body = "{\"status\": \"ok\"}\n";
        status = HTTP_OK;
    } else {
        status = HTTP_NOT_FOUND;
        body = "Not Found\n";
    }

    std::string response = build_response(body);

    ssize_t sent = 0;
    ssize_t total = response.size();
    while (sent < total && running_) {
        ssize_t r = write(client_fd, response.data() + sent, total - sent);
        if (r <= 0) break;
        sent += r;
    }

    close(client_fd);
}

std::string Server::build_response(const std::string& body) {
    std::string response = HTTP_OK;
    response += CONTENT_TYPE_JSON;
    response += CONNECTION_CLOSE;
    std::ostringstream ss;
    ss << "Content-Length: " << body.size();
    response += ss.str() + std::string(CRLF) + CRLF + body;
    return response;
}

std::string Server::snapshot_to_json(const DeviceSnapshot& snap) const {
    std::ostringstream ss;
    auto now = std::chrono::system_clock::now();
    auto epoch = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();

    ss << "{\n";
    ss << "  \"timestamp\": " << (long long)epoch << ",\n";
    ss << "  \"valid\": " << (snap.valid ? "true" : "false") << ",\n";
    ss << "  \"power\": {\n";
    ss << "    \"smart_meter_w\": " << snap.energy.smart_meter_power_w << ",\n";
    ss << "    \"ac_power_w\": " << snap.energy.ac_active_power_w << ",\n";
    ss << "    \"pv_power_w\": " << snap.energy.pv_total_power_w << ",\n";
    ss << "    \"battery_power_w\": " << snap.energy.battery_power_w << "\n";
    ss << "  },\n";
    ss << "  \"energy\": {\n";
    ss << "    \"charge_total_kwh\": " << snap.energy.total_charge_kwh << ",\n";
    ss << "    \"charge_today_kwh\": " << snap.energy.total_charge_today_kwh << ",\n";
    ss << "    \"discharge_total_kwh\": " << snap.energy.total_discharge_kwh << ",\n";
    ss << "    \"discharge_today_kwh\": " << snap.energy.total_discharge_today_kwh << ",\n";
    ss << "    \"feeder_total_kwh\": " << snap.energy.total_feeder_kwh << ",\n";
    ss << "    \"feeder_today_kwh\": " << snap.energy.total_feeder_today_kwh << ",\n";
    ss << "    \"consumption_total_kwh\": " << snap.energy.total_consumption_kwh << ",\n";
    ss << "    \"consumption_today_kwh\": " << snap.energy.total_consumption_today_kwh << ",\n";
    ss << "    \"output_total_kwh\": " << snap.energy.total_output_kwh << ",\n";
    ss << "    \"output_today_kwh\": " << snap.energy.total_output_today_kwh << ",\n";
    ss << "    \"load_total_kwh\": " << snap.energy.total_load_kwh << ",\n";
    ss << "    \"load_today_kwh\": " << snap.energy.total_load_today_kwh << "\n";
    ss << "  },\n";
    ss << "  \"battery\": {\n";
    ss << "    \"soc_pct\": " << snap.bms.soc << ",\n";
    ss << "    \"voltage_v\": " << snap.bms.voltage << ",\n";
    ss << "    \"current_a\": " << snap.bms.current << ",\n";
    ss << "    \"temperature_c\": " << snap.bms.temperature << "\n";
    ss << "  },\n";
    ss << "  \"meter1\": {\n";
    ss << "    \"connected\": " << (snap.meter1.connected ? "true" : "false") << ",\n";
    ss << "    \"voltage_v\": " << snap.meter1.r_voltage << ",\n";
    ss << "    \"current_a\": " << snap.meter1.r_current << ",\n";
    ss << "    \"power_w\": " << snap.meter1.combined_active_power << "\n";
    ss << "  },\n";
    ss << "  \"meter2\": {\n";
    ss << "    \"connected\": " << (snap.meter2.connected ? "true" : "false") << ",\n";
    ss << "    \"voltage_v\": " << snap.meter2.r_voltage << ",\n";
    ss << "    \"current_a\": " << snap.meter2.r_current << ",\n";
    ss << "    \"power_w\": " << snap.meter2.combined_active_power << "\n";
    ss << "  }\n";
    ss << "}\n";

    return ss.str();
}

} // namespace solakon::http
