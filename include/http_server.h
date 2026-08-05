#pragma once
/**
 * @file http_server.h
 * @brief Simple HTTP server that serves Solakon snapshot data as JSON.
 *
 * Listens on a TCP port and serves the latest snapshot at /data.
 * Used for Chrome plugins, dashboards, and API integration.
 */

#include "solakon_device.h"

#include <string>
#include <atomic>
#include <thread>

namespace solakon::http {

class Server {
public:
    Server(int port = 8080);
    ~Server();
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    /**
     * @brief Start the server in a background thread.
     * @return true if server started successfully.
     */
    bool start();

    /**
     * @brief Stop the server gracefully.
     */
    void stop();

    /**
     * @brief Update the latest snapshot (called from main loop).
     */
    void update_snapshot(const DeviceSnapshot& snap);

    /**
     * @brief Get the latest snapshot (thread-safe).
     */
    DeviceSnapshot get_snapshot() const;

    /**
     * @brief Check if the server is running.
     */
    bool is_running() const;

private:
    void listen_loop();
    void handle_client(int client_fd);
    std::string build_response(const std::string& body);
    std::string snapshot_to_json(const DeviceSnapshot& snap) const;

    int port_;
    int server_fd_ = -1;
    std::thread server_thread_;
    std::atomic<bool> running_{false};
    DeviceSnapshot latest_snapshot_;
    mutable std::mutex snapshot_mutex_;
};

} // namespace solakon::http
