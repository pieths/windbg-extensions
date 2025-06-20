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
#include <regex>
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
  MCPServer() : running_(false), server_socket_(INVALID_SOCKET), port_(0) {}
  ~MCPServer() { Stop(); }

  HRESULT Start(int port);
  HRESULT Stop();
  bool IsRunning() const { return running_; }
  int GetPort() const { return port_; }

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

  // WinDbg command handlers
  JSON ExecuteCommand(const JSON& params);
  JSON GetDebuggerState(const JSON& params);

  // Process all WinDbg commands on
  // the same thread sequentially
  void ProcessCommandQueue();
  void CommandProcessorThread();

  // Thread-safe wrapper for debug operations
  JSON ExecuteOnMainThread(std::function<JSON()> operation);

  // Member variables
  std::atomic<bool> running_;
  SOCKET server_socket_;
  int port_;
  std::thread server_thread_;

  // Add thread tracking for client socket handlers
  std::mutex clients_mutex_;
  std::set<std::thread::id> active_clients_;
  std::condition_variable clients_cv_;

  // Member variables to track client sockets
  std::set<SOCKET> client_sockets_;
  std::mutex client_sockets_mutex_;

  // Command queue for thread-safe operations
  struct DebugCommand {
    std::function<JSON()> operation;
    std::promise<JSON> result;
  };

  // Command queue which is used to process
  // all commands on a single thread.
  std::queue<std::unique_ptr<DebugCommand>> command_queue_;
  std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
  std::thread command_processor_thread_;
};

HRESULT MCPServer::Start(int port) {
  if (running_) {
    return E_FAIL;
  }

  // Initialize Winsock
  WSADATA wsa_data;
  if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
    return E_FAIL;
  }

  // Create socket
  server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket_ == INVALID_SOCKET) {
    WSACleanup();
    return E_FAIL;
  }

  // Bind socket
  sockaddr_in server_addr = {};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(server_socket_, (sockaddr*)&server_addr, sizeof(server_addr)) ==
      SOCKET_ERROR) {
    closesocket(server_socket_);
    WSACleanup();
    return E_FAIL;
  }

  // Listen
  if (listen(server_socket_, SOMAXCONN) == SOCKET_ERROR) {
    closesocket(server_socket_);
    WSACleanup();
    return E_FAIL;
  }

  // Get actual port if 0 was specified
  int addr_len = sizeof(server_addr);
  if (getsockname(server_socket_, (sockaddr*)&server_addr, &addr_len) == 0) {
    port_ = ntohs(server_addr.sin_port);
  } else {
    port_ = port;
  }

  // Start server thread
  running_ = true;
  server_thread_ = std::thread(&MCPServer::ServerThread, this);

  // Start command processor thread
  command_processor_thread_ =
      std::thread(&MCPServer::CommandProcessorThread, this);

  return S_OK;
}

HRESULT MCPServer::Stop() {
  if (!running_) {
    return S_OK;
  }

  running_ = false;
  queue_cv_.notify_all();  // Wake up command processor

  // Close server socket to unblock accept()
  if (server_socket_ != INVALID_SOCKET) {
    closesocket(server_socket_);
    server_socket_ = INVALID_SOCKET;
  }

  // Wait for server thread
  if (server_thread_.joinable()) {
    server_thread_.join();
  }

  // Wait for command processor thread
  if (command_processor_thread_.joinable()) {
    command_processor_thread_.join();
  }

  // Process any remaining commands
  ProcessCommandQueue();

  // Force close all client sockets to unblock recv() calls
  {
    std::lock_guard<std::mutex> lock(client_sockets_mutex_);
    for (SOCKET sock : client_sockets_) {
      shutdown(sock, SD_BOTH);
      closesocket(sock);
    }
  }

  // Wait for all client threads to finish
  {
    std::unique_lock<std::mutex> lock(clients_mutex_);
    clients_cv_.wait(lock, [this] { return active_clients_.empty(); });
  }

  WSACleanup();
  return S_OK;
}

void MCPServer::ServerThread() {
  while (running_) {
    sockaddr_in client_addr = {};
    int client_len = sizeof(client_addr);

    SOCKET client_socket =
        accept(server_socket_, (sockaddr*)&client_addr, &client_len);
    if (client_socket == INVALID_SOCKET) {
      if (running_) {
        // Real error, not shutdown
        Sleep(100);
      }
      continue;
    }

    // Handle client in separate thread, but track it
    std::thread client_thread([this, client_socket]() {
      // Register this thread and socket
      {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        active_clients_.insert(std::this_thread::get_id());
      }
      {
        std::lock_guard<std::mutex> lock(client_sockets_mutex_);
        client_sockets_.insert(client_socket);
      }

      ClientHandler(client_socket);

      // Unregister this thread and socket
      {
        std::lock_guard<std::mutex> lock(client_sockets_mutex_);
        client_sockets_.erase(client_socket);
      }
      {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        active_clients_.erase(std::this_thread::get_id());
        clients_cv_.notify_all();
      }
    });
    client_thread.detach();
  }
}

void MCPServer::ClientHandler(SOCKET client_socket) {
  char buffer[4096];
  std::string message_buffer;

  while (running_) {
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
       JSON::array({{{"name", "executeCommand"},
                     {"description", "Execute a WinDbg command"},
                     {"inputSchema",
                      {{"type", "object"},
                       {"properties",
                        {{"command",
                          {{"type", "string"},
                           {"description", "The WinDbg command to execute"}}}}},
                       {"required", JSON::array({"command"})}}}},
                    {{"name", "getDebuggerState"},
                     {"description", "Get the current debugger state"},
                     {"inputSchema",
                      {
                          {"type", "object"},
                          {"properties", JSON::object()}  // Explicitly create
                                                          // empty JSON object
                      }}}})}};
}

JSON MCPServer::HandleToolsCall(const JSON& params) {
  std::string tool_name = params.value("name", "");
  JSON arguments = params.value("arguments", JSON::object());

  if (tool_name == "executeCommand") {
    // Map to existing ExecuteCommand but wrap response in MCP format
    JSON result = ExecuteCommand(arguments);

    if (result.contains("error")) {
      // Error case
      return JSON{
          {"content",
           JSON::array(
               {{{"type", "text"},
                 {"text", "Error: " + result["error"].get<std::string>()}}})},
          {"isError", true}};
    } else {
      // Result is now just a string containing the output
      return JSON{
          {"content", JSON::array({{{"type", "text"},
                                    {"text", result.get<std::string>()}}})}};
    }
  } else if (tool_name == "getDebuggerState") {
    JSON result = GetDebuggerState(arguments);

    if (result.contains("error")) {
      // Error case
      return JSON{
          {"content",
           JSON::array(
               {{{"type", "text"},
                 {"text", "Error: " + result["error"].get<std::string>()}}})},
          {"isError", true}};
    } else {
      // Result is now just a string containing the output
      return JSON{
          {"content", JSON::array({{{"type", "text"},
                                    {"text", result.get<std::string>()}}})}};
    }
  } else {
    return JSON{
        {"content", JSON::array({{{"type", "text"},
                                  {"text", "Unknown tool: " + tool_name}}})},
        {"isError", true}};
  }
}

std::string GetCurrentContext() {
  std::string context_info;
  ULONG current_process_id = 0;
  ULONG current_thread_id = 0;
  static const ULONG INVALID_PROCESS_OR_THREAD_ID = static_cast<ULONG>(-1);

  HRESULT hr = g_debug.system_objects->GetCurrentProcessId(&current_process_id);
  if (FAILED(hr)) {
    current_process_id = INVALID_PROCESS_OR_THREAD_ID;
  }

  hr = g_debug.system_objects->GetCurrentThreadId(&current_thread_id);
  if (FAILED(hr)) {
    current_thread_id = INVALID_PROCESS_OR_THREAD_ID;
  }

  context_info += "Current Execution Context:\n";
  context_info += "  Process: " + std::to_string(current_process_id) + "\n";

  auto command_line = utils::ExecuteCommand(
      &g_debug,
      "dx -r0 @$curprocess.Environment.EnvironmentBlock.ProcessParameters->CommandLine.Buffer");
  size_t start_pos = command_line.find('"');
  size_t end_pos = command_line.rfind('"');
  if (start_pos != std::string::npos && end_pos != std::string::npos &&
      start_pos < end_pos) {
    // Extract the command line string
    command_line = command_line.substr(start_pos + 1, end_pos - start_pos - 1);
  } else {
    // If no quotes found, just use the whole line
    command_line = utils::Trim(command_line);
  }
  context_info += "  Process Command Line: " + command_line + "\n";

  context_info += "  Thread: " + std::to_string(current_thread_id) + "\n";

  // Get current source info
  utils::SourceInfo source_info = utils::GetCurrentSourceInfo(&g_debug);
  if (source_info.is_valid) {
    if (!source_info.file_path.empty()) {
      context_info += "  Source File: " + source_info.full_path + "\n";
      context_info +=
          "  Source Line: " + std::to_string(source_info.line) + "\n";
    }

    // Add source context if available
    if (!source_info.source_context.empty()) {
      // Add 4 spaces to the front of all lines in source_context
      std::string indented_context;
      std::istringstream stream(source_info.source_context);
      std::string line;
      bool first_line = true;

      while (std::getline(stream, line)) {
        if (!first_line) {
          indented_context += "\n";
        }
        indented_context += "    " + line;
        first_line = false;
      }

      context_info += "  Source Context (current line starts with \">\"):\n" +
                      indented_context + "\n";
    }
  }

  auto stack_trace = utils::GetTopOfCallStack(&g_debug);
  context_info += "  Call Stack (top 5):\n";
  // Add each stack frame from the vector
  for (size_t i = 0; i < stack_trace.size(); i++) {
    context_info += "    [" + std::to_string(i) + "] " + stack_trace[i] + "\n";
  }

  ULONG num_processes;
  g_debug.system_objects->GetNumberProcesses(&num_processes);

  if (!stack_trace.empty() &&
      stack_trace[0].find("ntdll!LdrpDoDebuggerBreak") != std::string::npos &&
      num_processes == 1) {
    context_info +=
        "\n"
        "Extra Context:\n"
        "This is a new debugging session and no commands have been executed yet.\n"
        "If you need to set breakpoints for child processes, use something\n"
        "similar to the following commands:\n"
        "\n"
        "    .childdbg 1; sxn ibp; sxn epr; sxe -c \"bp module_name!namespace_name::class_name::method_name; [...optionally more breakpoints if needed]; gc\" ld:module_name.dll; g\n"
        "\n"
        "    .childdbg 1; sxn ibp; sxn epr; sxe -c \"bp `module_name!D:\\\\path\\\\to\\\\file\\\\source_file.cc:42`; gc\" ld:module_name.dll; g\n"
        "\n"
        "This will set breakpoints in the child process for the specified module\n"
        "and start execution of the target. These commands should only be used\n"
        "for the initial breakpoints in a new debugging session.\n";
  }
  return context_info;
}

std::string GetPromptString() {
  ULONG current_process_id = 0;
  ULONG current_thread_id = 0;
  static const ULONG INVALID_PROCESS_OR_THREAD_ID = static_cast<ULONG>(-1);

  HRESULT hr = g_debug.system_objects->GetCurrentProcessId(&current_process_id);
  if (FAILED(hr)) {
    current_process_id = INVALID_PROCESS_OR_THREAD_ID;
  }

  hr = g_debug.system_objects->GetCurrentThreadId(&current_thread_id);
  if (FAILED(hr)) {
    current_thread_id = INVALID_PROCESS_OR_THREAD_ID;
  }

  char prompt[64];
  sprintf_s(prompt, sizeof(prompt), "%d:%03d> ", current_process_id,
            current_thread_id);
  return std::string(prompt);
}

// Run the specified WinDbg command and return the output.
// The output will always start with the original prompt+command and end with
// the new prompt. For example,
//
//      5:096> dv key_system
//          key_system = 0x00000027`bddfca40 "com.widevine.alpha"
//
//      5:096>
//
// Commands that do not produce output (like "g", "gu", "p", "t") will
// have the following extra context information added to the output. This
// extra context is also added when a breakpoint is hit.
//
//    5:096> p
//
//    Current Execution Context:
//      Process: 6
//      Process Command Line: "D:\cs\src\out\release_x64\chrome.exe" --type=u...
//      Thread: 92
//      Source File: D:\cs\src\media\mojo\services\media_foundation_service.cc
//      Source Line: 520
//      Source Context (current line starts with ">"):
//          516: }
//          517:
//          518: void MediaFoundationService::IsKeySystemSupported(
//          519:     const std::string& key_system,
//       >  520:     IsKeySystemSupportedCallback callback) {
//          521:   DVLOG(1) << __func__ << ": key_system=" << key_system;
//          522:
//          523:   SCOPED_UMA_HISTOGRAM_TIMER(
//          524:       "Media.EME.MediaFoundationService.IsKeySystemSupported");
//          525:
//      Call Stack (top 5):
//        [0] chrome!media::MediaFoundationService::IsKeySystemSupported
//        [1] chrome!media::mojom::MediaFoundationServiceStubDispatch::Accept...
//        [2] chrome!media::mojom::MediaFoundationServiceStub<mojo::RawPtrImp...
//        [3] chrome!mojo::InterfaceEndpointClient::HandleValidatedMessage
//        [4] chrome!mojo::InterfaceEndpointClient::HandleIncomingMessageThun...
//
//    5:096>
//
std::string ExecuteWinDbgCommand(const std::string& command) {
  std::string output;

  std::string prompt = GetPromptString();
  output += prompt + command + "\n";

  auto result = utils::ExecuteCommand(&g_debug, command, true);

  bool is_continuation_command =
      command == "g" || command == "gu" || command == "p" || command == "t";

  // Remove "ModLoad:" lines from all continuation commands except "g".
  if (is_continuation_command && command != "g") {
    std::string filtered_result;
    std::istringstream stream(result);
    std::string line;

    static const std::regex modload_regex(R"(^\s*ModLoad:)",
                                          std::regex::optimize);

    while (std::getline(stream, line)) {
      if (!std::regex_search(line, modload_regex)) {
        if (!filtered_result.empty()) {
          filtered_result += "\n";
        }
        filtered_result += line;
      }
    }

    result = filtered_result;
  }

  output += result;

  // Only add execution context when there is a possiblity that
  // the execution context has changed. This can happen for
  // continuation commands (like "g", "gu", "p", "t") or when
  // a breakpoint is hit.
  bool should_add_context = is_continuation_command;
  if (!should_add_context) {
    // Regex to match patterns like "Breakpoint 9 hit", "Breakpoint 123 hit".
    std::regex breakpoint_regex(R"(Breakpoint\s+\d+\s+hit)", std::regex::icase);
    if (std::regex_search(result, breakpoint_regex)) {
      should_add_context = true;
    }
  }

  if (should_add_context) {
    output += "\n\n" + GetCurrentContext();
  }

  output += "\n" + GetPromptString();
  return output;
}

JSON MCPServer::ExecuteCommand(const JSON& params) {
  std::string command = params.value("command", "");
  if (command.empty()) {
    return JSON{{"error", "No command specified"}};
  }

  return ExecuteOnMainThread([this, command]() {
    std::string output = ExecuteWinDbgCommand(command);
    return JSON(output);
  });
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

    std::string output = "Debugger State: " + state + "\n\n";
    output += GetCurrentContext() + "\n";
    output += GetPromptString();
    return JSON(output);
  });
}

JSON MCPServer::ExecuteOnMainThread(std::function<JSON()> operation) {
  auto cmd = std::make_unique<DebugCommand>();
  cmd->operation = operation;
  auto future = cmd->result.get_future();

  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    command_queue_.push(std::move(cmd));
  }
  // Wake up the command processor
  // thread to process the command
  queue_cv_.notify_one();

  // Retrieves the result of the asynchronous operation. This method blocks the
  // calling thread until the result is available.
  return future.get();
}

void MCPServer::ProcessCommandQueue() {
  std::unique_lock<std::mutex> lock(queue_mutex_);
  while (!command_queue_.empty()) {
    auto cmd = std::move(command_queue_.front());
    command_queue_.pop();
    lock.unlock();

    // Execute on main thread
    // Setting the value will unblock the future
    JSON result = cmd->operation();
    cmd->result.set_value(result);

    lock.lock();
  }
}

void MCPServer::CommandProcessorThread() {
  while (running_) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    queue_cv_.wait_for(lock, std::chrono::milliseconds(100),
                       [this] { return !command_queue_.empty() || !running_; });

    if (!running_) {
      break;
    }

    lock.unlock();
    ProcessCommandQueue();
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
        "Available MCP tools:\n"
        "  executeCommand     - Execute a debugger command\n"
        "  getDebuggerState   - Get debugger state\n\n"
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
