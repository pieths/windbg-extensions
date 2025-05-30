// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

// Prevent windows.h from including winsock.h to avoid
// duplicate definition errors when including winsock2.h
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#include <dbgeng.h>
#include <windows.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>

#include "json.hpp"
#include "utils.h"

// Include Ws2_32.lib for socket functions when linking
#pragma comment(lib, "Ws2_32.lib")

using JSON = nlohmann::json;

class MCPServer;
MCPServer* g_mcp_server = nullptr;

utils::DebugInterfaces g_debug;

class MCPServer {
 public:
  MCPServer() : m_running(false), m_server_socket(INVALID_SOCKET), m_port(0) {}
  ~MCPServer() { Stop(); }

  HRESULT Start(int port);
  HRESULT Stop();
  bool IsRunning() const { return m_running; }
  int GetPort() const { return m_port; }

 private:
  // Server management
  void ServerThread();
  void ClientHandler(SOCKET client_socket);

  // MCP protocol handlers
  JSON HandleRequest(const JSON& request);
  JSON CreateResponse(const JSON& id, const JSON& result);
  JSON CreateError(const JSON& id, int code, const std::string& message);
  JSON HandleInitialize(const JSON& params);
  JSON HandleToolsList(const JSON& params);
  JSON HandleToolsCall(const JSON& params);

  // Async operation management
  struct AsyncOperation {
    std::string id;
    std::string status;  // "pending", "completed", "error"
    JSON result;
    std::chrono::steady_clock::time_point start_time;
  };

  std::string GenerateOperationId();
  void StoreAsyncResult(const std::string& op_id, const JSON& result);
  JSON GetAsyncStatus(const std::string& op_id);

  // WinDbg command handlers
  JSON ExecuteCommand(const JSON& params);
  JSON GetOperationStatus(const JSON& params);
  JSON GetDebuggerState(const JSON& params);

  // Process commands on the main thread
  void ProcessCommandQueue();
  void CommandProcessorThread();

  // Thread-safe wrapper for debug operations
  JSON ExecuteOnMainThread(std::function<JSON()> operation);

  // Member variables
  std::atomic<bool> m_running;
  SOCKET m_server_socket;
  int m_port;
  std::thread m_server_thread;

  // Async operations are used to track long-running tasks.
  // A client can poll for the status of these operations.
  std::mutex m_operations_mutex;
  std::map<std::string, AsyncOperation> m_async_operations;
  static std::atomic<int> s_operation_counter;

  // Add thread tracking for client socket handlers
  std::mutex m_clients_mutex;
  std::set<std::thread::id> m_active_clients;
  std::condition_variable m_clients_cv;

  // Command queue for thread-safe operations
  struct DebugCommand {
    std::function<JSON()> operation;
    std::promise<JSON> result;
  };

  // Command queue which is used to process
  // all commands on a single thread.
  std::queue<std::unique_ptr<DebugCommand>> m_command_queue;
  std::mutex m_queue_mutex;
  std::condition_variable m_queue_cv;
  std::thread m_command_processor_thread;
};

std::atomic<int> MCPServer::s_operation_counter(0);

HRESULT MCPServer::Start(int port) {
  if (m_running) {
    return E_FAIL;
  }

  // Initialize Winsock
  WSADATA wsa_data;
  if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
    return E_FAIL;
  }

  // Create socket
  m_server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (m_server_socket == INVALID_SOCKET) {
    WSACleanup();
    return E_FAIL;
  }

  // Bind socket
  sockaddr_in server_addr = {};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(m_server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) ==
      SOCKET_ERROR) {
    closesocket(m_server_socket);
    WSACleanup();
    return E_FAIL;
  }

  // Listen
  if (listen(m_server_socket, SOMAXCONN) == SOCKET_ERROR) {
    closesocket(m_server_socket);
    WSACleanup();
    return E_FAIL;
  }

  // Get actual port if 0 was specified
  int addr_len = sizeof(server_addr);
  if (getsockname(m_server_socket, (sockaddr*)&server_addr, &addr_len) == 0) {
    m_port = ntohs(server_addr.sin_port);
  } else {
    m_port = port;
  }

  // Start server thread
  m_running = true;
  m_server_thread = std::thread(&MCPServer::ServerThread, this);

  // Start command processor thread
  m_command_processor_thread =
      std::thread(&MCPServer::CommandProcessorThread, this);

  return S_OK;
}

HRESULT MCPServer::Stop() {
  if (!m_running) {
    return S_OK;
  }

  m_running = false;
  m_queue_cv.notify_all();  // Wake up command processor

  // Close server socket to unblock accept()
  if (m_server_socket != INVALID_SOCKET) {
    closesocket(m_server_socket);
    m_server_socket = INVALID_SOCKET;
  }

  // Wait for server thread
  if (m_server_thread.joinable()) {
    m_server_thread.join();
  }

  // Wait for command processor thread
  if (m_command_processor_thread.joinable()) {
    m_command_processor_thread.join();
  }

  // Process any remaining commands
  ProcessCommandQueue();

  // Wait for all client threads to finish
  {
    std::unique_lock<std::mutex> lock(m_clients_mutex);
    m_clients_cv.wait(lock, [this] { return m_active_clients.empty(); });
  }

  WSACleanup();
  return S_OK;
}

void MCPServer::ServerThread() {
  while (m_running) {
    sockaddr_in client_addr = {};
    int client_len = sizeof(client_addr);

    SOCKET client_socket =
        accept(m_server_socket, (sockaddr*)&client_addr, &client_len);
    if (client_socket == INVALID_SOCKET) {
      if (m_running) {
        // Real error, not shutdown
        Sleep(100);
      }
      continue;
    }

    // Handle client in separate thread, but track it
    std::thread client_thread([this, client_socket]() {
      // Register this thread
      {
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        m_active_clients.insert(std::this_thread::get_id());
      }

      ClientHandler(client_socket);

      // Unregister this thread
      {
        std::lock_guard<std::mutex> lock(m_clients_mutex);
        m_active_clients.erase(std::this_thread::get_id());
        m_clients_cv.notify_all();
      }
    });
    client_thread.detach();
  }
}

void MCPServer::ClientHandler(SOCKET client_socket) {
  char buffer[4096];
  std::string message_buffer;

  while (m_running) {
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
      break;
    }

    message_buffer.append(buffer, bytes_received);

    // Simple message framing: messages end with \n
    size_t pos;
    while ((pos = message_buffer.find('\n')) != std::string::npos) {
      std::string message = message_buffer.substr(0, pos);
      message_buffer.erase(0, pos + 1);

      try {
        JSON request = JSON::parse(message);
        JSON response = HandleRequest(request);

        std::string response_str = response.dump() + "\n";
        send(client_socket, response_str.c_str(), (int)response_str.length(),
             0);
      } catch (const std::exception& e) {
        JSON error =
            // JSON-RPC error response
            // -32700 - Parse error: Invalid JSON was received by the server
            CreateError("", -32700, std::string("Parse error: ") + e.what());
        std::string error_str = error.dump() + "\n";
        send(client_socket, error_str.c_str(), (int)error_str.length(), 0);
      }
    }
  }

  closesocket(client_socket);
}

// This method is run from one of the client handler threads
JSON MCPServer::HandleRequest(const JSON& request) {
  try {
    std::string method = request.value("method", "");
    JSON params = request.value("params", JSON::object());

    // Handle id as JSON value (can be string, number, or null)
    JSON id = nullptr;
    if (request.contains("id")) {
      id = request["id"];
    }

    // MCP Protocol methods
    if (method == "initialize") {
      return CreateResponse(id, HandleInitialize(params));
    } else if (method == "initialized") {
      // This is a notification, no response needed
      return JSON::object();
    } else if (method == "tools/list") {
      return CreateResponse(id, HandleToolsList(params));
    } else if (method == "tools/call") {
      return CreateResponse(id, HandleToolsCall(params));
    } else {
      return CreateError(id, -32601, "Method not found: " + method);
    }
  } catch (const std::exception& e) {
    return CreateError(nullptr, -32603,
                       std::string("Internal error: ") + e.what());
  }
}

JSON MCPServer::CreateResponse(const JSON& id, const JSON& result) {
  JSON response = {{"jsonrpc", "2.0"}, {"result", result}};
  if (!id.is_null()) {
    response["id"] = id;
  }
  return response;
}

JSON MCPServer::CreateError(const JSON& id,
                            int code,
                            const std::string& message) {
  JSON response = {{"jsonrpc", "2.0"},
                   {"error", {{"code", code}, {"message", message}}}};
  if (!id.is_null()) {
    response["id"] = id;
  }
  return response;
}

std::string MCPServer::GenerateOperationId() {
  int counter = s_operation_counter.fetch_add(1);
  return "op_" + std::to_string(counter) + "_" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count());
}

void MCPServer::StoreAsyncResult(const std::string& op_id, const JSON& result) {
  std::lock_guard<std::mutex> lock(m_operations_mutex);
  auto it = m_async_operations.find(op_id);
  if (it != m_async_operations.end()) {
    it->second.status = "completed";
    it->second.result = result;
  }
}

JSON MCPServer::GetAsyncStatus(const std::string& op_id) {
  std::lock_guard<std::mutex> lock(m_operations_mutex);
  auto it = m_async_operations.find(op_id);
  if (it != m_async_operations.end()) {
    JSON status = {{"operation_id", op_id}, {"status", it->second.status}};

    if (it->second.status == "completed" || it->second.status == "error") {
      status["result"] = it->second.result;

      // Clean up old completed operations
      auto age = std::chrono::steady_clock::now() - it->second.start_time;
      if (age > std::chrono::minutes(5)) {
        m_async_operations.erase(it);
      }
    }

    return status;
  }

  return {{"operation_id", op_id}, {"status", "not_found"}};
}

JSON MCPServer::GetOperationStatus(const JSON& params) {
  std::string operation_id = params.value("operation_id", "");
  if (operation_id.empty()) {
    return JSON{{"error", "No operation_id specified"}};
  }

  return GetAsyncStatus(operation_id);
}

JSON MCPServer::ExecuteCommand(const JSON& params) {
  std::string command = params.value("command", "");
  if (command.empty()) {
    return JSON{{"error", "No command specified"}};
  }

  std::string op_id = GenerateOperationId();

  // Store operation so that future requests can poll for its status
  {
    std::lock_guard<std::mutex> lock(m_operations_mutex);
    m_async_operations[op_id] = {op_id, "pending", JSON::object(),
                                 std::chrono::steady_clock::now()};
  }

  // Queue the operation to run on the command processor thread
  auto cmd = std::make_unique<DebugCommand>();
  cmd->operation = [this, op_id, command]() {
    std::string output = utils::ExecuteCommand(&g_debug, command,
                                               /*wait_for_break_status=*/true);

    // No manual escaping needed - nlohmann::json handles it
    JSON result = {
        {"command", command},
        {"output", output}  // Newlines will be escaped automatically
    };

    StoreAsyncResult(op_id, result);
    return JSON{{"queued", true}};
  };

  {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_command_queue.push(std::move(cmd));
  }
  // Wake up the command processor
  // thread to process the command
  m_queue_cv.notify_one();

  // Don't wait for the result, return immediately
  return JSON{{"operation_id", op_id},
              {"status", "pending"},
              {"type", "async_operation"},
              {"metadata",
               {{"async", true},
                {"poll_method", "getOperationStatus"},
                {"poll_interval_ms", 100}}}};
}

JSON MCPServer::GetDebuggerState(const JSON& params) {
  return ExecuteOnMainThread([this]() {
    ULONG status = 0;
    HRESULT hr = g_debug.control->GetExecutionStatus(&status);

    if (FAILED(hr)) {
      return JSON{{"error", "Failed to get execution status"}};
    }

    std::string state;
    switch (status) {
      case DEBUG_STATUS_NO_DEBUGGEE:
        state = "no_debuggee";
        break;
      case DEBUG_STATUS_BREAK:
        state = "break";
        break;
      case DEBUG_STATUS_STEP_OVER:
      case DEBUG_STATUS_STEP_INTO:
      case DEBUG_STATUS_STEP_BRANCH:
        state = "stepping";
        break;
      case DEBUG_STATUS_GO:
      case DEBUG_STATUS_GO_HANDLED:
      case DEBUG_STATUS_GO_NOT_HANDLED:
        state = "running";
        break;
      default:
        state = "unknown";
        break;
    }

    return JSON{{"state", state}, {"status_code", status}};
  });
}

void MCPServer::ProcessCommandQueue() {
  std::unique_lock<std::mutex> lock(m_queue_mutex);
  while (!m_command_queue.empty()) {
    auto cmd = std::move(m_command_queue.front());
    m_command_queue.pop();
    lock.unlock();

    // Execute on main thread
    // Setting the value will unblock the future
    JSON result = cmd->operation();
    cmd->result.set_value(result);

    lock.lock();
  }
}

void MCPServer::CommandProcessorThread() {
  while (m_running) {
    std::unique_lock<std::mutex> lock(m_queue_mutex);
    m_queue_cv.wait_for(lock, std::chrono::milliseconds(100), [this] {
      return !m_command_queue.empty() || !m_running;
    });

    if (!m_running) {
      break;
    }

    lock.unlock();
    ProcessCommandQueue();
  }
}

JSON MCPServer::ExecuteOnMainThread(std::function<JSON()> operation) {
  auto cmd = std::make_unique<DebugCommand>();
  cmd->operation = operation;
  auto future = cmd->result.get_future();

  {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_command_queue.push(std::move(cmd));
  }
  // Wake up the command processor
  // thread to process the command
  m_queue_cv.notify_one();

  // Retrieves the result of the asynchronous operation. This method blocks the
  // calling thread until the result is available.
  return future.get();
}

JSON MCPServer::HandleInitialize(const JSON& params) {
  // MCP initialization response
  return JSON{
      {"protocolVersion", "0.1.0"},
      {"capabilities", {{"tools", {{"listChanged", true}}}, {"prompts", {}}}},
      {"serverInfo", {{"name", "windbg-mcp-server"}, {"version", "1.0.0"}}}};
}

JSON MCPServer::HandleToolsList(const JSON& params) {
  // Return list of available tools in MCP format
  return JSON{
      {"tools",
       JSON::array(
           {{{"name", "executeCommand"},
             {"description", "Execute a WinDbg command asynchronously"},
             {"inputSchema",
              {{"type", "object"},
               {"properties",
                {{"command",
                  {{"type", "string"},
                   {"description", "The WinDbg command to execute"}}}}},
               {"required", JSON::array({"command"})}}}},
            {{"name", "getOperationStatus"},
             {"description", "Check the status of an asynchronous operation"},
             {"inputSchema",
              {{"type", "object"},
               {"properties",
                {{"operation_id",
                  {{"type", "string"},
                   {"description", "The operation ID to check"}}}}},
               {"required", JSON::array({"operation_id"})}}}},
            {{"name", "getDebuggerState"},
             {"description", "Get the current debugger state"},
             {"inputSchema",
              {
                  {"type", "object"},
                  {"properties",
                   JSON::object()}  // Explicitly create empty JSON object
              }}}})}};
}

JSON MCPServer::HandleToolsCall(const JSON& params) {
  std::string tool_name = params.value("name", "");
  JSON arguments = params.value("arguments", JSON::object());

  if (tool_name == "executeCommand") {
    // Map to existing ExecuteCommand but wrap response in MCP format
    JSON result = ExecuteCommand(arguments);

    // Check if it's an async operation
    if (result.contains("operation_id")) {
      // For async operations, return the operation info
      return JSON{
          {"content",
           JSON::array(
               {{{"type", "text"},
                 {"text", "Command queued for execution. Operation ID: " +
                              result["operation_id"].get<std::string>()}}})},
          {"meta", result}  // Include full operation details in meta
      };
    } else if (result.contains("error")) {
      // Error case
      return JSON{
          {"content",
           JSON::array(
               {{{"type", "text"},
                 {"text", "Error: " + result["error"].get<std::string>()}}})},
          {"isError", true}};
    } else {
      // Synchronous result (shouldn't happen with current implementation)
      return JSON{{"content",
                   JSON::array({{{"type", "text"}, {"text", result.dump()}}})}};
    }
  } else if (tool_name == "getOperationStatus") {
    JSON result = GetOperationStatus(arguments);

    // Format the response for MCP
    std::string status_text;
    if (result.contains("status")) {
      std::string status = result["status"];
      if (status == "completed" && result.contains("result")) {
        status_text = "Operation completed:\n" +
                      result["result"]["output"].get<std::string>();
      } else if (status == "pending") {
        status_text = "Operation is still pending...";
      } else if (status == "not_found") {
        status_text = "Operation not found";
      } else {
        status_text = "Operation status: " + status;
      }
    } else if (result.contains("error")) {
      status_text = "Error: " + result["error"].get<std::string>();
    }

    return JSON{
        {"content", JSON::array({{{"type", "text"}, {"text", status_text}}})}};
  } else if (tool_name == "getDebuggerState") {
    JSON result = GetDebuggerState(arguments);

    std::string state_text;
    if (result.contains("state")) {
      state_text = "Debugger state: " + result["state"].get<std::string>();
    } else if (result.contains("error")) {
      state_text = "Error: " + result["error"].get<std::string>();
    }

    return JSON{
        {"content", JSON::array({{{"type", "text"}, {"text", state_text}}})}};
  } else {
    return JSON{
        {"content", JSON::array({{{"type", "text"},
                                  {"text", "Unknown tool: " + tool_name}}})},
        {"isError", true}};
  }
}

HRESULT CALLBACK StartMCPServerInternal(IDebugClient* client,
                                        const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "EXPERIMENTAL.\n"
        "StartMCPServer - Start an MCP (Model Context Protocol) server\n\n"
        "Usage: !StartMCPServer [port]\n\n"
        "  port - TCP port to listen on (default: 0 for automatic)\n\n"
        "The MCP server allows AI assistants and other tools to interact\n"
        "with WinDbg through a JSON-RPC protocol.\n\n"
        "Available MCP methods:\n"
        "  windbg.executeCommand     - Execute a debugger command asynchronously\n"
        "  windbg.getOperationStatus - Check async operation status\n"
        "  windbg.getDebuggerState   - Get debugger state\n\n"
        "Examples:\n"
        "  !StartMCPServer        - Start on automatic port\n"
        "  !StartMCPServer 8080   - Start on port 8080\n"
        "  !StartMCPServer ?      - Show this help\n\n");
    return S_OK;
  }

  if (g_mcp_server && g_mcp_server->IsRunning()) {
    DOUT("MCP server is already running on port %d\n", g_mcp_server->GetPort());
    return S_OK;
  }

  int port = 0;
  if (args && *args) {
    std::vector<std::string> parsed_args = utils::ParseCommandLine(args);

    if (!parsed_args.empty()) {
      if (parsed_args.size() > 1) {
        DERROR("Error: Too many arguments. Expected 0 or 1 argument.\n");
        return E_INVALIDARG;
      }

      std::string port_str = utils::Trim(parsed_args[0]);

      if (!utils::IsWholeNumber(port_str)) {
        DERROR("Error: Port must be a valid number.\n");
        return E_INVALIDARG;
      }

      port = std::stoi(port_str);
      if (port < 0 || port > 65535) {
        DERROR("Invalid port number: %s. Port must be between 0 and 65535.\n",
               port_str.c_str());
        return E_INVALIDARG;
      }
    }
  }

  if (!g_mcp_server) {
    g_mcp_server = new MCPServer();
  }

  HRESULT hr = g_mcp_server->Start(port);
  if (SUCCEEDED(hr)) {
    DOUT("MCP server started on port %d\n", g_mcp_server->GetPort());
    DOUT("Connect using: tcp://localhost:%d\n", g_mcp_server->GetPort());
  } else {
    DERROR("Failed to start MCP server\n");
  }

  return hr;
}

HRESULT CALLBACK StopMCPServerInternal(IDebugClient* client, const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "StopMCPServer - Stop the MCP server\n\n"
        "Usage: !StopMCPServer\n\n"
        "This command stops the running MCP server.\n\n");
    return S_OK;
  }

  if (!g_mcp_server || !g_mcp_server->IsRunning()) {
    DOUT("MCP server is not running\n");
    return S_OK;
  }

  HRESULT hr = g_mcp_server->Stop();
  if (SUCCEEDED(hr)) {
    DOUT("MCP server stopped\n");
  } else {
    DERROR("Failed to stop MCP server\n");
  }

  return hr;
}

HRESULT CALLBACK MCPServerStatusInternal(IDebugClient* client,
                                         const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "MCPServerStatus - Show MCP server status\n\n"
        "Usage: !MCPServerStatus\n\n"
        "This command displays the current status of the MCP server.\n\n");
    return S_OK;
  }

  if (!g_mcp_server) {
    DOUT("MCP server has not been initialized\n");
  } else if (g_mcp_server->IsRunning()) {
    DOUT("MCP server is running on port %d\n", g_mcp_server->GetPort());
    DOUT("Connect using: tcp://localhost:%d\n", g_mcp_server->GetPort());
  } else {
    DOUT("MCP server is stopped\n");
  }

  return S_OK;
}

HRESULT CALLBACK DebugExtensionInitializeInternal(PULONG version,
                                                  PULONG flags) {
  *version = DEBUG_EXTENSION_VERSION(1, 0);
  *flags = 0;
  return utils::InitializeDebugInterfaces(&g_debug);
}

HRESULT CALLBACK DebugExtensionUninitializeInternal() {
  if (g_mcp_server) {
    g_mcp_server->Stop();
    delete g_mcp_server;
    g_mcp_server = nullptr;
  }

  return utils::UninitializeDebugInterfaces(&g_debug);
}

extern "C" {
__declspec(dllexport) HRESULT CALLBACK DebugExtensionInitialize(PULONG version,
                                                                PULONG flags) {
  return DebugExtensionInitializeInternal(version, flags);
}

__declspec(dllexport) HRESULT CALLBACK DebugExtensionUninitialize(void) {
  return DebugExtensionUninitializeInternal();
}

__declspec(dllexport) HRESULT CALLBACK StartMCPServer(IDebugClient* client,
                                                      const char* args) {
  return StartMCPServerInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK StopMCPServer(IDebugClient* client,
                                                     const char* args) {
  return StopMCPServerInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK MCPServerStatus(IDebugClient* client,
                                                       const char* args) {
  return MCPServerStatusInternal(client, args);
}
}
