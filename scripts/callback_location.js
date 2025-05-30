// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

"use strict";

function __PrintSymbol(functorHexString, symbol) {
    const pattern = /([^[]+)\s+\[([^@]+)\s+@\s+(\d+)\]:/;
    const match = pattern.exec(symbol);

    if (!match) {
        host.diagnostics.debugLog("Failed to parse symbol: " + symbol + "\n");
        return;
    }

    const functionName = match[1].trim();
    const fileName = match[2].trim();
    const lineNumber = parseInt(match[3], 10);

    // Replace all the backslashes in filename with double backslashes
    // so that it can be copy and pasted back into a command.
    const escapedFileName = fileName.replace(/\\/g, "\\\\");

    let outputString = "\nbp " + functorHexString + "\n";
    outputString += "bp /1 " + functorHexString + "\n\n";
    outputString += "The following symbol was found for the given functor:\n";
    outputString += "Note, this symbol might not be correct.\n";
    outputString += "Use the hex address provided above for the actual callback location when unsure.\n\n";
    outputString += functionName + "\n";
    outputString += fileName + ":" + lineNumber + "\n";
    outputString += escapedFileName + ":" + lineNumber + "\n";
    outputString += "`" + escapedFileName + ":" + lineNumber + "`\n\n";
    host.diagnostics.debugLog(outputString);
}

function __RunOptionalCommand(optional_command, functorHexString) {
    if (!optional_command) {
        return;
    }

    if (typeof optional_command !== "string" || optional_command.trim() === "") {
        host.diagnostics.debugLog("Invalid command provided: " + optional_command + "\n");
        return;
    }

    if (optional_command === "bp") {
        host.diagnostics.debugLog("Setting a breakpoint on the functor: " + functorHexString + "\n");
        host.namespace.Debugger.Utility.Control.ExecuteCommand("bp " + functorHexString);
    } else if (optional_command === "bpg") {
        host.diagnostics.debugLog("Setting a breakpoint on the functor: " + functorHexString + "\n");
        host.diagnostics.debugLog("Continuing execution...\n");
        host.namespace.Debugger.Utility.Control.ExecuteCommand("bp " + functorHexString);
        host.namespace.Debugger.Utility.Control.ExecuteCommand("g");

    } else if (optional_command === "bp1") {
        host.diagnostics.debugLog("Setting a one-time breakpoint on the functor: " + functorHexString + "\n");
        host.namespace.Debugger.Utility.Control.ExecuteCommand("bp /1 " + functorHexString);
    } else if (optional_command === "bp1g") {
        host.diagnostics.debugLog("Setting a one-time breakpoint on the functor: " + functorHexString + "\n");
        host.diagnostics.debugLog("Continuing execution...\n");
        host.namespace.Debugger.Utility.Control.ExecuteCommand("bp /1 " + functorHexString);
        host.namespace.Debugger.Utility.Control.ExecuteCommand("g");

    } else {
        host.diagnostics.debugLog("Invalid command provided: " + optional_command + "\n\n");
        return;
    }

    host.diagnostics.debugLog("\n");
}

function GetCallbackLocation(callback, optional_command) {
    let ctl = host.namespace.Debugger.Utility.Control;
    let bindStateBaseSize = ctl.ExecuteCommand("dx sizeof(chrome!base::internal::BindStateBase)");
    bindStateBaseSize = parseInt(bindStateBaseSize[0].split(" ").at(-1).trim(), 16);

    let bindStateBaseAddress = callback.holder_.bind_state_.Ptr.address;

    // Get the start address of the BindState member variables.
    // This should be right after all the BindStateBase member
    // variables since BindState is a subclass of BindStateBase.
    let bindStateMembersAddress = bindStateBaseAddress + bindStateBaseSize;

    // The functor (function pointer) is the first member of BindState
    let functor = host.memory.readMemoryValues(bindStateMembersAddress, 1, 8)[0];
    let functorHexString = "0x" + functor.toString(16);

    // Get the symbol which is pointed to by the functor
    let symbol = ctl.ExecuteCommand("uf " + functorHexString);
    if (!symbol[0].includes("!base::internal::BindPostTaskTrampoline")) {
        // This should be the symbol for callbacks that
        // are not created with BindPostTask (BindPostTaskToCurrentDefault).
        __PrintSymbol(functorHexString, symbol[0]);
        __RunOptionalCommand(optional_command, functorHexString);
        return;
    }

    // The functor points to BindPostTaskTrampoline<...>::Run().
    // In this scenario, the bound_args which comes right after
    // the functor is a pointer to the BindPostTaskTrampoline instance.
    let trampolinePointer = host.memory.readMemoryValues(bindStateMembersAddress + 8, 1, 8)[0];

    // BindPostTaskTrampoline has three member variables:
    //   const scoped_refptr<TaskRunner> task_runner_;
    //   const Location location_;
    //   CallbackType callback_;
    // Where the callback contains the actual callback.
    // See base\task\bind_post_task_internal.h for more details.

    // Start by adding the size of the first member variable
    // scoped_refptr<TaskRunner> to the trampoline pointer.
    let callbackAddress = trampolinePointer + 8;

    // The second member variable is the Location object.
    let locationSize = ctl.ExecuteCommand("dx sizeof(chrome!base::Location)");
    locationSize = parseInt(locationSize[0].split(" ").at(-1).trim(), 16);
    callbackAddress += locationSize;

    // The callback instance (both Once and Repeating)
    // contains a BindStateHolder.
    // See base\functional\callback.h for more details.
    // The BindStateHolder contains a pointer to the BindState instance.
    // See base\functional\callback_internal.h for more details.
    // So, the pointer which is located at the callbackAddress
    // is a pointer to the original BindState instance of the actual callback.

    // Get the address of the BindState instance and do the same
    // as above to get the address of the functor and the symbol.
    bindStateBaseAddress = host.memory.readMemoryValues(callbackAddress, 1, 8)[0];
    bindStateMembersAddress = bindStateBaseAddress + bindStateBaseSize;
    functor = host.memory.readMemoryValues(bindStateMembersAddress, 1, 8)[0];
    functorHexString = "0x" + functor.toString(16);
    symbol = ctl.ExecuteCommand("uf " + functorHexString);
    __PrintSymbol(functorHexString, symbol[0]);
    __RunOptionalCommand(optional_command, functorHexString);
}
