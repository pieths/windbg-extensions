// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

"use strict";


function __ConvertTickAddressToInt64(tickAddress) {
    // A tick address looks like "00007ff9`9e89a160"
    // Before parsing it to an Int64, we need to clean it up
    // by removing the backtick and leading zeros.
    let tickAddressCleaned = tickAddress.replace(/`/g, "").replace(/^0+/, "");
    let result = host.parseInt64(tickAddressCleaned, 16);
    return result;
}

function __GetCurrentSourceInfo() {
    let sourceFile = host.namespace.Debugger.State.DebuggerVariables.curframe.Attributes.SourceInformation.SourceFile;
    let sourceLine = host.namespace.Debugger.State.DebuggerVariables.curframe.Attributes.SourceInformation.SourceLine;
    let module = host.namespace.Debugger.State.DebuggerVariables.curframe.Attributes.SourceInformation.Module.Name;
    sourceLine = parseInt(sourceLine, 16);
    return { line: sourceLine, file: sourceFile, module: module };
}

function __GetCurrentFunctionCallLocations() {
    let ctl = host.namespace.Debugger.Utility.Control;
    let ufCommandOutput = ctl.ExecuteCommand("uf /c @rip");
    let calls = [];

    // Regular expressions to extract information
    const callRegex = /^\s*(.*?)\+0x([0-9a-f]+)\s+\((.*?)\)\s+\[(.*?)\s+@\s+(\d+)\]:\s*$/;
    const targetRegex = /^\s*call to\s+(.*?)\s+\(([0-9a-fA-F`]{8,}?)\)\s+\[(.*?)\s+@\s+(\d+)\]$/;

    let currentCall = null;

    for (let line of ufCommandOutput) {
        let lineText = line.toString().trim();

        // Parse call site information
        let callMatch = callRegex.exec(lineText);
        if (callMatch) {
            // Create a new call site object
            currentCall = {
                sourceLocation: {
                    symbol: callMatch[1],
                    offset: "0x" + callMatch[2],
                    address: callMatch[3],
                    sourceFile: callMatch[4],
                    sourceLine: parseInt(callMatch[5])
                },
                targetLocation: null
            };

            // Add the call to the list regardless
            // of whether it has a target
            calls.push(currentCall);
            continue;
        }

        // Parse target function information
        let targetMatch = targetRegex.exec(lineText);
        if (targetMatch && currentCall) {
            currentCall.targetLocation = {
                symbol: targetMatch[1],
                address: targetMatch[2],
                sourceFile: targetMatch[3],
                sourceLine: parseInt(targetMatch[4])
            };

            currentCall = null;
        }
    }

    return calls;
}

function __CompareCurrentIpToTarget(targetTickAddress) {
    // Get an Int64 version of the current IP (instruction pointer)
    let currentIp = host.namespace.Debugger.State.DebuggerVariables.curframe.Attributes.InstructionOffset;
    let targetIp = __ConvertTickAddressToInt64(targetTickAddress);
    return currentIp.compareTo(targetIp);
}

// Steps into the last function call at the current
// source line which is not a destructor.
// If a text pattern is provided, the calls on the
// specified line are filtered based on that pattern
// and the last matching call is stepped into.
// The pattern matching is case insensitive.
function __StepIntoFunctionAtLine(lineNumber, textPattern) {
    let ctl = host.namespace.Debugger.Utility.Control;
    let calls = __GetCurrentFunctionCallLocations();

    calls = calls.filter(call =>
        call.sourceLocation.sourceLine === lineNumber
        && (call.targetLocation === null ||
            !call.targetLocation.symbol.includes("::~"))
    );

    if (textPattern) {
        // Filter calls based on the provided text pattern
        textPattern = textPattern.toLowerCase();
        calls = calls.filter(call =>
            call.targetLocation !== null &&
            call.targetLocation.symbol.toLowerCase().includes(textPattern));
    }

    if (calls.length > 0) {
        // Get the last matching call
        let call = calls[calls.length - 1];

        let compare = __CompareCurrentIpToTarget(call.sourceLocation.address);
        if (compare === 0) {
            // If the current IP is already at the desired
            // function call then just step into it.
            host.diagnostics.debugLog("Current IP is already at the desired function call.\n");
            ctl.ExecuteCommand("t");
            return;
        } else if (compare < 0) {
            // If the current IP is before the desired
            // function call then set a breakpoint and run.
            ctl.ExecuteCommand("~. bp /1 " + call.sourceLocation.address + " \"t\"");
            ctl.ExecuteCommand("g");
            return;
        } else {
            host.diagnostics.debugLog("Current IP is after the desired function call.\n");
            return;
        }
    }

    host.diagnostics.debugLog("No function call found on the current line.\n");
    return;
}

function __StepIntoNamedFunction(textPattern) {
    let ctl = host.namespace.Debugger.Utility.Control;
    let calls = __GetCurrentFunctionCallLocations();

    textPattern = textPattern.toLowerCase();

    let matchingCalls = calls.filter(call =>
        call.targetLocation !== null &&
        call.targetLocation.symbol.toLowerCase().includes(textPattern) &&
        !call.targetLocation.symbol.includes("::~") &&
        __CompareCurrentIpToTarget(call.sourceLocation.address) < 0
    );

    if (matchingCalls.length > 0) {
        // Get the first matching call
        let call = matchingCalls[0];

        ctl.ExecuteCommand("~. bp /1 " + call.sourceLocation.address + " \"t\"");
        ctl.ExecuteCommand("g");
        return;
    }

    host.diagnostics.debugLog("No matching function call found after the current instruction pointer.\n");
    return;
}

function StepIntoFunction(input1, input2) {
    if (input1 === undefined || input1 === null) {
        // If input1 is undefined or null then step into the last
        // function on the current line which is not a destructor.
        let currentLine = __GetCurrentSourceInfo().line;
        __StepIntoFunctionAtLine(currentLine);
    } else if (typeof input1 === "number") {
        if (typeof input2 === "string") {
            // If input2 is a string then step into the input1 line
            // number and use the string as a text pattern to filter
            // the calls on that line. Step into the last matching
            // function ignoring destructors.
            __StepIntoFunctionAtLine(input1, input2);
        } else {
            // If input1 is a number and there is no text pattern
            // then step into the last function at the given line
            // number which is not a destructor.
            __StepIntoFunctionAtLine(input1);
        }
    } else if (typeof input1 === "string") {
        __StepIntoNamedFunction(input1);
    }
}

function Go(input1) {
    let ctl = host.namespace.Debugger.Utility.Control;

    if (typeof input1 === "number") {
        // If the input is a number then set a breakpoint at that line
        // in the current source file and continue execution.
        let ctl = host.namespace.Debugger.Utility.Control;
        let currentSourceInfo = __GetCurrentSourceInfo();
        let escapedSourceFile = currentSourceInfo.file.replace(/\\/g, "\\\\");
        let module = currentSourceInfo.module.split("\\").pop().replace(/\.[^.]+$/, "");

        let bpString = "~. bp /1 " + "`" + module + "!" + escapedSourceFile +
                       ":" + input1 + "`";
        ctl.ExecuteCommand(bpString);
    }

    ctl.ExecuteCommand("g");
}
