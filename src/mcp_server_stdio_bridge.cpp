// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

#include <winsock2.h>
#include <ws2tcpip.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

class StdioBridge {
 public:
  StdioBridge(const std::string& host, int port)
      : host_(host), port_(port), running_(false), connected_(false) {}

  bool Connect() {
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
      return false;
    }

    return AttemptConnection();
  }

  void Run() {
    running_ = true;

    // Start the stdin reader thread
    std::thread stdin_thread([this]() {
      std::string line;
      while (running_ && std::getline(std::cin, line)) {
        std::unique_lock<std::mutex> lock(stdin_mutex_);
        stdin_buffer_.push_back(line);
        stdin_cv_.notify_one();
      }
      running_ = false;
    });

    // Main loop with reconnection logic
    while (running_) {
      if (!connected_) {
        std::cerr << "Attempting to reconnect to MCP server..." << std::endl;
        if (!AttemptConnection()) {
          std::cerr << "Failed to connect. Retrying in 5 seconds..."
                    << std::endl;
          std::this_thread::sleep_for(std::chrono::seconds(5));
          continue;
        }
        std::cerr << "Successfully reconnected to MCP server" << std::endl;
      }

      // Start threads for connected communication
      std::thread send_thread([this]() { SendLoop(); });

      std::thread receive_thread([this]() { ReceiveLoop(); });

      // Wait for either thread to finish (indicating disconnection)
      send_thread.join();
      receive_thread.join();

      // Connection was lost
      connected_ = false;
      if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
      }

      if (running_) {
        std::cerr
            << "Connection lost. Will attempt to reconnect in 5 seconds..."
            << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
      }
    }

    stdin_thread.join();
  }

  ~StdioBridge() {
    running_ = false;
    connected_ = false;
    stdin_cv_.notify_all();

    if (socket_ != INVALID_SOCKET) {
      closesocket(socket_);
    }
    WSACleanup();
  }

 private:
  bool AttemptConnection() {
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ == INVALID_SOCKET) {
      return false;
    }

    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr);

    if (connect(socket_, (sockaddr*)&server_addr, sizeof(server_addr)) ==
        SOCKET_ERROR) {
      closesocket(socket_);
      socket_ = INVALID_SOCKET;
      return false;
    }

    connected_ = true;
    return true;
  }

  void SendLoop() {
    while (running_ && connected_) {
      std::unique_lock<std::mutex> lock(stdin_mutex_);
      stdin_cv_.wait_for(lock, std::chrono::milliseconds(100), [this] {
        return !stdin_buffer_.empty() || !running_;
      });

      while (!stdin_buffer_.empty() && connected_) {
        std::string line = stdin_buffer_.front();
        stdin_buffer_.erase(stdin_buffer_.begin());
        lock.unlock();

        line += '\n';
        int result =
            send(socket_, line.c_str(), static_cast<int>(line.length()), 0);
        if (result == SOCKET_ERROR) {
          connected_ = false;
          break;
        }

        lock.lock();
      }
    }
  }

  void ReceiveLoop() {
    char buffer[4096];
    while (running_ && connected_) {
      int bytes_received = recv(socket_, buffer, sizeof(buffer) - 1, 0);
      if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        std::cout << buffer;
        std::cout.flush();
      } else {
        // Connection closed or error
        connected_ = false;
        break;
      }
    }
  }

  std::string host_;
  int port_;
  SOCKET socket_ = INVALID_SOCKET;

  std::atomic<bool> running_;
  std::atomic<bool> connected_;

  std::mutex stdin_mutex_;
  std::condition_variable stdin_cv_;
  std::vector<std::string> stdin_buffer_;
};

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: mcp_stdio_bridge <host> <port>" << std::endl;
    return 1;
  }

  StdioBridge bridge(argv[1], std::stoi(argv[2]));
  if (!bridge.Connect()) {
    std::cerr << "Initial connection failed. Will keep trying..." << std::endl;
  }

  bridge.Run();
  return 0;
}
