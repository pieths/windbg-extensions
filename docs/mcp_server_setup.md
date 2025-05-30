# MCP Server Setup for Visual Studio Code

This guide explains how to set up the WinDbg MCP (Model Context Protocol) server to work with Visual Studio Code's GitHub Copilot when debugging Chromium.

## Overview

The MCP server allows GitHub Copilot to interact directly with WinDbg through a structured API, enabling AI-assisted debugging workflows. Since VS Code doesn't support direct TCP connections to MCP servers, we use a bridge executable (`mcp_stdio_bridge.exe`) to facilitate communication.

## Prerequisites

- Visual Studio Code with GitHub Copilot
- This WinDbg extensions project built (run `.\build.ps1`)
- Chromium source code opened in VS Code

## Setup Instructions

### 1. Copy Copilot Instructions to Chromium Project

Copy the WinDbg-specific instructions to your Chromium project's instructions directory:

```
Copy: docs\windbg_mcp_chromium.instructions.md
To:   <chromium_project>\.github\instructions\windbg_mcp_chromium.instructions.md
```

These instructions provide Copilot with:
- Detailed information about available WinDbg commands and APIs
- Chromium-specific debugging guidance
- Best practices for debugging multi-process Chrome applications

**Note**: This file is actively maintained. Update it as needed based on your debugging workflows and Copilot's performance.

### 2. Enable MCP Support in VS Code

Add the following settings to your Chromium project's workspace settings (`.vscode/settings.json`):

```json
{
    "chat.promptFiles": true,
    "chat.mcp.enabled": true
}
```

If the file doesn't exist, create it with these settings.

### 3. Configure MCP Server Connection

Create or update `.vscode/mcp.json` in your Chromium project with the following configuration:

```json
{
    "servers": {
        "windbg_mcp": {
            "type": "stdio",
            "command": "D:\\windbg\\extensions\\build_output\\mcp_stdio_bridge.exe",
            "args": ["127.0.0.1", "8099"]
        }
    }
}
```

**Important**:
Update the path to match your actual extensions directory location.
Update the port (8099) to match the port which is displayed when the
MCP server starts inside of WinDbg.


### 4. Start the MCP Server

In WinDbg (with your extensions loaded) and the Chromium process started,
start the MCP server:

```
!StartMCPServer
```

To check the server status run the following command:
```
!MCPServerStatus
```

To stop the server:
```
!StopMCPServer
```

## Usage

Once configured, you can use GitHub Copilot Chat in VS Code to:

- Set breakpoints in Chromium code
- Execute WinDbg commands through natural language
- Get debugging assistance with stack traces and variable inspection
- Navigate complex Chromium debugging scenarios

Example Copilot prompts:
- "Set a breakpoint in the media CDM creation code"
- "Show me the current call stack"
- "What are the local variables in this function?"

## Troubleshooting

**MCP Server Not Connecting:**
- Verify the path in `mcp.json` is correct
- Ensure `mcp_stdio_bridge.exe` exists in `build_output/`
- Check that the MCP server is running in WinDbg (`!MCPServerStatus`)

**Copilot Not Using WinDbg Commands:**
- Confirm `windbg_mcp_chromium.instructions.md` is in the correct location
- Verify MCP settings are enabled in VS Code
- Restart VS Code after configuration changes (shouldn't be necessary but might help)

**Command Execution Issues:**
- Check WinDbg output for error messages
- Use `!MCPServerStatus` to verify server health

## Additional Resources

- [Complete Commands Reference](commands.md)
- [MCP Protocol Documentation](windbg_mcp_chromium.instructions.md)
- [WinDbg Extensions Development Guide](windbg_extensions_guide.md)
