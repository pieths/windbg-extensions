// Copyright (c) 2025 Piet Hein Schouten
// SPDX-License-Identifier: MIT

// This extension provides a way to step through Mojo messages.
// It patches the mojo::InterfaceEndpointClient::HandleValidatedMessage
// method to check if the message flags have bit 29 set. If this bit
// is set then it breaks into the debugger and steps through the mojo code
// until one of the Accept* endpoint methods is encountered. Patching is
// used here instead of a conditional breakpoint because there are enough
// messages occurring all the time that it would be too slow to use a
// conditional breakpoint.
//
// Location of the message flags in relation to the mojo::Message instance.
//
// 7:106> dx message
// message                 : 0x11ba5fd190 [Type: mojo::Message *]
//     kFlagExpectsResponse : 0x1 [Type: unsigned int]
//     kFlagIsResponse  : 0x2 [Type: unsigned int]
//     kFlagIsSync      : 0x4 [Type: unsigned int]
//     kFlagNoInterrupt : 0x8 [Type: unsigned int]
//     kFlagIsUrgent    : 0x10 [Type: unsigned int]
//     [+0x000] handle_          [Type: mojo::ScopedHandleBase<mojo::Message
//     [+0x008] payload_buffer_  [Type: mojo::internal::Buffer]
//     [+0x030] handles_         : { size=0 } [Type: std::__Cr::vector<mojo:
//     [+0x048] associated_endpoint_handles_ : { size=0 } [Type: std::__Cr::
//     [+0x060] receiver_connection_group_ [Type: base::raw_ptr<const mojo::
//     [+0x068] transferable_    : true [Type: bool]
//     [+0x069] serialized_      : true [Type: bool]
//     [+0x070] heap_profiler_tag_ : 0x0 [Type: char *]
//     [+0x078] interface_name_  : 0x0 [Type: char *]
//     [+0x080] method_name_     : 0x0 [Type: char *]
// 7:106> dx sizeof(message->payload_buffer_)
// sizeof(message->payload_buffer_) : 0x28
// 7:106> dx -id 0,7 -r1 (*((chrome!mojo::internal::Buffer *)0x11ba5fd198))
// (*((chrome!mojo::internal::Buffer *)0x11ba5fd198))  [Type: mojo::internal
//     [+0x000] message_         [Type: mojo::MessageHandle]
//     [+0x008] message_payload_size_ : 0x0 [Type: unsigned __int64]
//     [+0x010] data_            : 0xb6c000f02d0 [Type: void *]
//     [+0x018] size_            : 0x78 [Type: unsigned __int64]
//     [+0x020] cursor_          : 0x78 [Type: unsigned __int64]
// 7:106> dq 0x11ba5fd190                                 <== mojo::Message*
// 00000011`ba5fd190  00000b6c`00054d80 00000000`00000000
// 00000011`ba5fd1a0  00000000`00000000 00000b6c`000f02d0 <== data_ pointer
// 00000011`ba5fd1b0  00000000`00000078 00000000`00000078
// 00000011`ba5fd1c0  00000000`00000000 00000000`00000000
// 00000011`ba5fd1d0  00000000`00000000 00000000`00000000
// 00000011`ba5fd1e0  00000000`00000000 00000000`00000000
// 00000011`ba5fd1f0  00000b6c`0013f9c8 aaaaaaaa`aaaa0101
// 00000011`ba5fd200  00000000`00000000 00000000`00000000
// 7:106> dd 00000b6c`000f02d0
// 00000b6c`000f02d0  00000038 00000003 00000000 00000000
// 00000b6c`000f02e0  20000001 5374a5a2 00000001 00000000 <== flags at offset
// 00000b6c`000f02f0  00000018 00000000 00000000 00000000  0x10; 5th dword;
// 00000b6c`000f0300  00000000 00000000 00000010 00000000  0x20000001
// 00000b6c`000f0310  00000008 00000000 0000002e 00000026
// 00000b6c`000f0320  2e6d6f63 7263696d 666f736f 6c702e74
// 00000b6c`000f0330  65727961 2e796461 6f636572 6e656d6d
// 00000b6c`000f0340  69746164 00006e6f efbeadde 0dd0feca

#include <dbgeng.h>
#include <windows.h>
#include <functional>
#include <iomanip>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "debug_event_callbacks.h"
#include "utils.h"

utils::DebugInterfaces g_debug;

// The names of the modules that should be patched.
static std::vector<std::string> g_modules;
static const std::string kHandleValidatedMessageSymbol =
    "mojo::InterfaceEndpointClient::HandleValidatedMessage";

class HookDefinition;

struct HookInstance {
  ULONG process_id = 0;
  ULONG64 target_address = 0;
  ULONG64 hook_address = 0;
  ULONG hook_allocated_memory_size = 0;
  ULONG64 jump_target = 0;
  ULONG64 int3_address = 0;
  const HookDefinition* hook_definition = nullptr;
  std::string module_name;
};

static std::vector<HookInstance> g_hook_instances;

class HookDefinition {
 public:
  virtual bool CheckSignature(ULONG64 target_address) const = 0;
  virtual bool ApplyHook(HookInstance& hook_instance) const = 0;
  virtual int NumStepsToHookExit() const = 0;
  virtual std::string GetName() const = 0;
  virtual ~HookDefinition() = default;

 protected:
  // Convience method which does a byte for byte comparison.
  bool CheckSignature(ULONG64 target_address,
                      const std::vector<BYTE> expected_bytes) const {
    // Read the current bytes at the target address
    std::vector<BYTE> current_bytes(expected_bytes.size());
    ULONG bytes_read;
    HRESULT hr = g_debug.data_spaces->ReadVirtual(
        target_address, current_bytes.data(),
        static_cast<ULONG>(current_bytes.size()), &bytes_read);

    if (FAILED(hr) || bytes_read != current_bytes.size()) {
      DOUT("Failed to read bytes from target address\n");
      return false;
    }

    // Compare the bytes
    if (memcmp(current_bytes.data(), expected_bytes.data(),
               expected_bytes.size()) != 0) {
      return false;
    }

    return true;
  }

  // Convience method to get the bytes that will be used to patch the
  // original function. This is a jump instruction that will redirect
  // execution to the hook address.
  std::vector<BYTE> GetPatchBytes(ULONG64 hook_address, size_t size) const {
    std::vector<BYTE> patch_bytes = {
        // jmp qword ptr [rip+0]
        0xFF,
        0x25,
        0x00,
        0x00,
        0x00,
        0x00,
        // Next 8 bytes will be the absolute address to jump to
        static_cast<BYTE>(hook_address & 0xFF),
        static_cast<BYTE>((hook_address >> 8) & 0xFF),
        static_cast<BYTE>((hook_address >> 16) & 0xFF),
        static_cast<BYTE>((hook_address >> 24) & 0xFF),
        static_cast<BYTE>((hook_address >> 32) & 0xFF),
        static_cast<BYTE>((hook_address >> 40) & 0xFF),
        static_cast<BYTE>((hook_address >> 48) & 0xFF),
        static_cast<BYTE>((hook_address >> 56) & 0xFF),
    };

    // If the requested size is larger than our patch, fill with NOPs
    if (size > patch_bytes.size()) {
      patch_bytes.resize(size, 0x90);  // 0x90 is the NOP instruction
    }
    return patch_bytes;
  }

  // Convience method to allocate memory for the hook code
  HRESULT AllocateHookMemory(HookInstance& hook_instance, size_t size) const {
    std::string command = ".dvalloc " + std::to_string(size);
    std::string output = utils::ExecuteCommand(&g_debug, command, true);

    // 0:000> .dvalloc 100
    // Allocated 1000 bytes starting at 0000024f`c5a70000
    //
    // Parse the allocation result to get the address and size
    size_t pos = output.find("Allocated ");
    if (pos != std::string::npos) {
      // Extract the number of bytes allocated
      pos += 10;  // Skip "Allocated "
      size_t bytes_end = output.find(" bytes", pos);
      if (bytes_end != std::string::npos) {
        std::string bytes_str = output.substr(pos, bytes_end - pos);

        // Parse the allocated size (it's in hex)
        hook_instance.hook_allocated_memory_size =
            static_cast<ULONG>(std::stoull(bytes_str, nullptr, 16));

        // Now find the address
        pos = output.find("starting at ", bytes_end);
        if (pos != std::string::npos) {
          pos += 12;  // Skip "starting at "
          size_t end = output.find_first_not_of("0123456789abcdefABCDEF`", pos);
          std::string addr_str = output.substr(pos, end - pos);

          // Remove backtick if present
          size_t backtick = addr_str.find('`');
          if (backtick != std::string::npos) {
            addr_str.erase(backtick, 1);
          }

          hook_instance.hook_address = std::stoull(addr_str, nullptr, 16);
          return S_OK;
        }
      }
    }

    return E_FAIL;
  }
};

// Release build with no config changes to the "bindings"
// component in src\mojo\public\cpp\bindings\BUILD.gn
class HookReleaseNoConfigChanges : public HookDefinition {
 public:
  virtual bool CheckSignature(ULONG64 target_address) const override {
    // This hook can do a direct byte comparison
    return HookDefinition::CheckSignature(target_address, original_code_);
  }

  virtual bool ApplyHook(HookInstance& hook_instance) const override {
    AllocateHookMemory(hook_instance, hook_code_.size());

    hook_instance.int3_address = hook_instance.hook_address + int3_offset_;

    // Calculate the jump target (original function + size of replaced bytes)
    ULONG64 jump_target = hook_instance.target_address + original_code_.size();
    hook_instance.jump_target = jump_target;

    // Write the jump target address into the hook code and
    // write the hook code to the allocated memory
    std::vector<BYTE> hook_bytes = hook_code_;
    size_t jump_target_offset =
        hook_bytes.size() - hook_code_jump_target_offset_;
    for (int i = 0; i < 8; i++) {
      hook_bytes[jump_target_offset + i] = (jump_target >> (i * 8)) & 0xFF;
    }
    ULONG bytes_written;
    g_debug.data_spaces->WriteVirtual(
        hook_instance.hook_address, hook_bytes.data(),
        static_cast<ULONG>(hook_bytes.size()), &bytes_written);

    // Patch the original function with a jump to the hook
    std::vector<BYTE> patch_bytes =
        GetPatchBytes(hook_instance.hook_address, original_code_.size());

    g_debug.data_spaces->WriteVirtual(
        hook_instance.target_address, patch_bytes.data(),
        static_cast<ULONG>(patch_bytes.size()), &bytes_written);
    return true;
  }

  virtual int NumStepsToHookExit() const override {
    return num_steps_to_hook_exit;
  }

  virtual std::string GetName() const override {
    return "HookReleaseNoConfigChanges";
  }

 private:
  const std::vector<BYTE> original_code_ = {
      0x41, 0x57,                               // push r15
      0x41, 0x56,                               // push r14
      0x41, 0x54,                               // push r12
      0x56,                                     // push rsi
      0x57,                                     // push rdi
      0x55,                                     // push rbp
      0x53,                                     // push rbx
      0x48, 0x81, 0xEC, 0xF0, 0x01, 0x00, 0x00  // sub rsp, 1F0h

  };
  const std::vector<BYTE> hook_code_ = {
      // Save registers we'll use
      0x50,  // push rax
      0x51,  // push rcx
      0x52,  // push rdx
      0x53,  // push rbx

      // Load the message pointer from RDX (second parameter)
      0x48, 0x89, 0xD0,  // mov rax, rdx
      0x48, 0x85, 0xC0,  // test rax, rax
      0x74, 0x1A,        // jz skip_check (jump to pop rbx at offset 0x26)

      // Get the data_ pointer from payload_buffer_
      // (which is inline at offset 0x8)
      // data_ is at offset 0x10 within payload_buffer_, so total offset
      // is 0x18
      0x48, 0x8B, 0x40, 0x18,  // mov rax, [rax+18h]  ; get data_ pointer
      0x48, 0x85, 0xC0,        // test rax, rax
      0x74, 0x11,              // jz skip_check ; jump to pop rbx at offset 0x26

      // Check the flags (5th dword from start of data)
      0x8B, 0x40, 0x10,              // mov eax, [rax+10h] ; get flags
      0x25, 0x00, 0x00, 0x00, 0x20,  // and eax, 20000000h ; check bit 29
      0x74, 0x07,                    // jz skip_check      ; jump to pop rbx
                                     //                      at offset 0x26

      // Break into debugger
      0xCC,                                // int3; At offset 0x1F (31)
      0x90, 0x90, 0x90, 0x90, 0x90, 0x90,  // nops for alignment

      // skip_check: (offset 0x26)
      // Restore registers
      0x5B,  // pop rbx
      0x5A,  // pop rdx
      0x59,  // pop rcx
      0x58,  // pop rax

      // Execute original instructions (from the disassembly)
      0x41, 0x57,                                // push r15
      0x41, 0x56,                                // push r14
      0x41, 0x54,                                // push r12
      0x56,                                      // push rsi
      0x57,                                      // push rdi
      0x55,                                      // push rbp
      0x53,                                      // push rbx
      0x48, 0x81, 0xEC, 0xF0, 0x01, 0x00, 0x00,  // sub rsp, 1F0h

      // Jump to original function + 17 bytes
      // (after the instructions we copied)
      0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00,       // mov rax, imm64 (placeholder)
      0xFF, 0xE0  // jmp rax
  };
  const int hook_code_jump_target_offset_ = 10;
  const int int3_offset_ = 31;
  const int num_steps_to_hook_exit = 21;
};

// Release build with config set to no_optimize for the
// "bindings" component in src\mojo\public\cpp\bindings\BUILD.gn
class HookReleaseWithConfigNoOptimize : public HookDefinition {
 public:
  virtual bool CheckSignature(ULONG64 target_address) const override {
    // Original code:
    // chrome!mojo::InterfaceEndpointClient::HandleValidatedMessage
    // 00007ffe`a6062ce0 4881ec78070000  sub     rsp,778h
    // 00007ffe`a6062ce7 488b0592b8640d  mov     rax,qword ptr
    //                                           [chrome!__security_cookie
    //                                            (00007ffe`b36ae580)]
    // 00007ffe`a6062cee 4831e0          xor     rax,rsp
    //
    // Read enough bytes to check the pattern up to and including xor rax, rsp
    const size_t bytes_to_read = 17;  // Covers up to the xor instruction
    std::vector<BYTE> actual_bytes(bytes_to_read);
    ULONG bytes_read;
    HRESULT hr = g_debug.data_spaces->ReadVirtual(
        target_address, actual_bytes.data(),
        static_cast<ULONG>(actual_bytes.size()), &bytes_read);

    if (FAILED(hr) || bytes_read != actual_bytes.size()) {
      DOUT("Failed to read bytes from target address\n");
      return false;
    }

    // Check sub rsp, imm32 pattern (first 3 bytes of opcode)
    // The instruction is: 48 81 EC [4-byte immediate]
    if (actual_bytes[0] != 0x48 || actual_bytes[1] != 0x81 ||
        actual_bytes[2] != 0xEC) {
      return false;
    }
    // Skip checking the 4-byte immediate (bytes 3-6) as it varies

    // Check mov rax, qword ptr [rip+offset] pattern (3 bytes for opcode)
    // The instruction is: 48 8B 05 [4-byte offset]
    if (actual_bytes[7] != 0x48 || actual_bytes[8] != 0x8B ||
        actual_bytes[9] != 0x05) {
      return false;
    }
    // Skip checking the 4-byte offset (bytes 10-13) as it varies

    // Check xor rax, rsp (3 bytes)
    if (actual_bytes[14] != 0x48 || actual_bytes[15] != 0x31 ||
        actual_bytes[16] != 0xE0) {
      return false;
    }

    return true;
  }

  virtual bool ApplyHook(HookInstance& hook_instance) const override {
    // Read the original bytes we're going to replace (17 bytes)
    std::vector<BYTE> original_bytes(original_code_size_);
    ULONG bytes_read;
    HRESULT hr = g_debug.data_spaces->ReadVirtual(
        hook_instance.target_address, original_bytes.data(),
        static_cast<ULONG>(original_bytes.size()), &bytes_read);

    if (FAILED(hr) || bytes_read != original_bytes.size()) {
      DERROR("Failed to read original bytes from target address\n");
      return false;
    }

    // Allocate memory for the hook
    hr = AllocateHookMemory(hook_instance, hook_code_.size());
    if (FAILED(hr)) {
      DERROR("Failed to allocate hook memory\n");
      return false;
    }

    hook_instance.int3_address = hook_instance.hook_address + int3_offset_;

    // Calculate the jump target (original function + size of replaced bytes)
    ULONG64 jump_target = hook_instance.target_address + original_code_size_;
    hook_instance.jump_target = jump_target;

    // Create a copy of the hook code
    std::vector<BYTE> hook_bytes = hook_code_;

    // Extract the RIP-relative offset from the original instruction
    int32_t rip_offset = *reinterpret_cast<int32_t*>(&original_bytes[10]);

    // Calculate the absolute address of the security cookie
    // In the original code, RIP points to the next instruction (offset 14)
    ULONG64 security_cookie_addr =
        hook_instance.target_address + 14 + rip_offset;

    // First, copy the sub rsp instruction
    for (size_t i = 0; i < 7; i++) {
      hook_bytes[0x2A + i] = original_bytes[i];
    }

    // Write the 64-bit security cookie address into the hardcoded mov rax,
    // imm64
    for (int i = 0; i < 8; i++) {
      hook_bytes[0x33 + i] = (security_cookie_addr >> (i * 8)) & 0xFF;
    }

    // Write the jump target address into the hook code
    size_t jump_target_offset =
        hook_bytes.size() - hook_code_jump_target_offset_;
    for (int i = 0; i < 8; i++) {
      hook_bytes[jump_target_offset + i] = (jump_target >> (i * 8)) & 0xFF;
    }

    // Write the hook code to the allocated memory
    ULONG bytes_written;
    hr = g_debug.data_spaces->WriteVirtual(
        hook_instance.hook_address, hook_bytes.data(),
        static_cast<ULONG>(hook_bytes.size()), &bytes_written);

    if (FAILED(hr) || bytes_written != hook_bytes.size()) {
      DERROR("Failed to write hook code\n");
      return false;
    }

    // Patch the original function with a jump to the hook
    std::vector<BYTE> patch_bytes =
        GetPatchBytes(hook_instance.hook_address, original_code_size_);

    hr = g_debug.data_spaces->WriteVirtual(
        hook_instance.target_address, patch_bytes.data(),
        static_cast<ULONG>(patch_bytes.size()), &bytes_written);

    if (FAILED(hr) || bytes_written != patch_bytes.size()) {
      DERROR("Failed to patch original function\n");
      return false;
    }

    return true;
  }

  virtual int NumStepsToHookExit() const override {
    return num_steps_to_hook_exit_;
  }

  virtual std::string GetName() const override {
    return "HookReleaseWithConfigNoOptimize";
  }

 private:
  const size_t original_code_size_ = 17;  // We'll replace 17 bytes
  const std::vector<BYTE> hook_code_ = {
      // Save registers we'll use
      0x50,  // push rax
      0x51,  // push rcx
      0x52,  // push rdx
      0x53,  // push rbx

      // Load the message pointer from RDX (second parameter)
      0x48, 0x89, 0xD0,  // mov rax, rdx
      0x48, 0x85, 0xC0,  // test rax, rax
      0x74, 0x1A,        // jz skip_check (jump to pop rbx at offset 0x26)

      // Get the data_ pointer from payload_buffer_
      // (which is inline at offset 0x8)
      // data_ is at offset 0x10 within payload_buffer_, so total offset
      // is 0x18
      0x48, 0x8B, 0x40, 0x18,  // mov rax, [rax+18h]  ; get data_ pointer
      0x48, 0x85, 0xC0,        // test rax, rax
      0x74, 0x11,              // jz skip_check ; jump to pop rbx at offset 0x26

      // Check the flags (5th dword from start of data)
      0x8B, 0x40, 0x10,              // mov eax, [rax+10h] ; get flags
      0x25, 0x00, 0x00, 0x00, 0x20,  // and eax, 20000000h ; check bit 29
      0x74, 0x07,                    // jz skip_check      ; jump to pop rbx
                                     //                      at offset 0x26

      // Break into debugger
      0xCC,                                // int3; At offset 0x1F (31)
      0x90, 0x90, 0x90, 0x90, 0x90, 0x90,  // nops for alignment

      // skip_check: (offset 0x26)
      // Restore registers
      0x5B,  // pop rbx
      0x5A,  // pop rdx
      0x59,  // pop rcx
      0x58,  // pop rax

      // Execute original instructions (offset 0x2A; 42)
      // sub rsp, imm32 (7 bytes) - will be dynamically copied
      0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,

      // mov rax, imm64 (10 bytes). Address at offset 0x33 (51)
      0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      // mov rax, [rax] (3 bytes)
      0x48, 0x8B, 0x00,

      // xor rax, rsp (3 bytes)
      0x48, 0x31, 0xE0,

      // Jump to original function + 17 bytes (after the instructions we copied)
      // using R11 instead of something like RAX since RAX is used immediately
      // following the original instructions we copied and would thus get
      // corrupted and cause issues with the security cookie check.
      0x49, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00,             // mov r11, imm64 (placeholder)
      0x41, 0xFF, 0xE3  // jmp r11
  };
  const int hook_code_jump_target_offset_ = 11;
  const int int3_offset_ = 31;
  const int num_steps_to_hook_exit_ = 17;
};

static std::vector<HookDefinition*> g_hook_definitions = {
    new HookReleaseNoConfigChanges(), new HookReleaseWithConfigNoOptimize()};

// Forward declarations
void StepOutOfHook(const HookInstance* hook_instance);
void StepThroughHandleValidatedMessage();
void PatchModule(const std::string& module_name);

class EventCallbacks : public DebugEventCallbacks {
 public:
  EventCallbacks()
      : DebugEventCallbacks(DEBUG_EVENT_CHANGE_ENGINE_STATE |
                            DEBUG_EVENT_LOAD_MODULE) {}

  STDMETHOD(ChangeEngineState)(ULONG flags, ULONG64 argument) {
    if (flags & DEBUG_CES_EXECUTION_STATUS) {
      // Check if the target was suspended.
      // DEBUG_STATUS_BREAK indicates that the target is suspended
      // for any reason. This includes things like step into and step over.
      if (argument == DEBUG_STATUS_BREAK && !g_hook_instances.empty()) {
        ULONG64 current_ip = 0;
        HRESULT hr = g_debug.registers->GetInstructionOffset(&current_ip);
        if (FAILED(hr)) {
          return S_OK;
        }

        // Check if this breakpoint is one of our hook int3s
        for (const auto& hook_instance : g_hook_instances) {
          if (current_ip == hook_instance.int3_address) {
            DOUT("Mojo Hook breakpoint hit at %p for process %u\n", current_ip,
                 hook_instance.process_id);

            try {
              StepOutOfHook(&hook_instance);
              StepThroughHandleValidatedMessage();
            } catch (const std::exception& e) {
              DERROR("Error while stepping through hook: %s\n", e.what());
            }
          }
        }
      }
    }
    return S_OK;
  }

  STDMETHOD(LoadModule)(ULONG64 ImageFileHandle,
                        ULONG64 BaseOffset,
                        ULONG ModuleSize,
                        PCSTR ModuleName,
                        PCSTR ImageName,
                        ULONG CheckSum,
                        ULONG TimeDateStamp) {
    // ModuleName: "chrome"
    // ImageName: "D:\cs\src\out\release_x64\chrome.dll"
    std::string module_name(ModuleName);
    if (!module_name.ends_with(".dll")) {
      module_name += ".dll";
    }

    for (const auto& target_module : g_modules) {
      if (module_name == target_module) {
        PatchModule(module_name);
        return DEBUG_STATUS_NO_CHANGE;
      }
    }
    return DEBUG_STATUS_NO_CHANGE;
  }
};

static EventCallbacks* g_event_callbacks = nullptr;

void StepOutOfHook(const HookInstance* hook_instance) {
  if (!hook_instance || !hook_instance->hook_definition) {
    DOUT("Invalid hook instance\n");
    return;
  }

  utils::DebugContextGuard debug_context_guard(&g_debug);
  int num_steps = hook_instance->hook_definition->NumStepsToHookExit();

  // Execute the number of steps needed to exit the hook
  for (int i = 0; i < num_steps; i++) {
    g_debug.control->SetExecutionStatus(DEBUG_STATUS_STEP_OVER);
    g_debug.control->WaitForEvent(0, INFINITE);
    debug_context_guard.RestoreIfChanged();
  }
}

void StepThroughHandleValidatedMessage() {
  utils::DebugContextGuard debug_context_guard(&g_debug);
  const int kMaxFrameDepth = 6;
  const int kMaxSteps = 200;

  for (int i = 0; i < kMaxSteps; i++) {
    utils::SourceInfo source_info = utils::GetCurrentSourceInfo(&g_debug);
    std::vector<std::string> call_stack =
        utils::GetTopOfCallStack(&g_debug, kMaxFrameDepth);

    // Get the current frame depth based on either the HandleValidatedMessage
    // method or one of the Accept methods. The Accept methods that are
    // referred to here are the ones that are located in the corresponding
    // *.mojom.h files. All three of these methods require stepping into to
    // get to the target method in the *.mojom.cc file.
    size_t frame_depth = 0;
    for (const auto& symbol : call_stack) {
      if (symbol.ends_with(kHandleValidatedMessageSymbol) ||
          symbol.ends_with("::Accept") ||
          symbol.ends_with("::AcceptWithResponder")) {
        break;
      }
      frame_depth++;
    }

    std::string top_of_stack = call_stack[0];

    if ((top_of_stack.ends_with("::Accept") ||
         top_of_stack.ends_with("::AcceptWithResponder")) &&
        source_info.file_name.ends_with(".mojom.cc")) {
      // Stepped into a mojo endpoint Accept method.
      // This is the final target where the execution should break.
      DOUT("Found mojo Accept method in %d steps.\n", i);
      break;
    } else if (frame_depth == kMaxFrameDepth) {
      DERROR("Reached maximum frame depth without finding accept method.\n");
      break;
    } else if (frame_depth >= 2) {
      // We stepped in too far. Step out to the previous frame.
      utils::ExecuteCommand(&g_debug, "gu", true);
      debug_context_guard.RestoreIfChanged();
    } else if (frame_depth == 1) {
      // Inside a method that was called from HandleValidatedMessage or
      // one of the Accept methods in *.mojom.h and this is not the
      // target method yet (which is checked in the first condition).
      // Continue stepping until we are out of the current method.
      g_debug.control->SetExecutionStatus(DEBUG_STATUS_STEP_OVER);
      g_debug.control->WaitForEvent(0, INFINITE);
      debug_context_guard.RestoreIfChanged();
    } else if (frame_depth == 0) {
      g_debug.control->SetExecutionStatus(DEBUG_STATUS_STEP_INTO);
      g_debug.control->WaitForEvent(0, INFINITE);
      debug_context_guard.RestoreIfChanged();
    }
  }
}

void StepIntoMessageAndSetFlag() {
  utils::DebugContextGuard debug_context_guard(&g_debug);

  utils::SourceInfo source_info = utils::GetCurrentSourceInfo(&g_debug);
  if (!source_info.file_name.ends_with(".mojom.cc")) {
    DERROR("Error: Current source file is not a .mojom.cc file.\n");
    return;
  }

  // Matches Message::Message followed by optional whitespace and opening
  // parenthesis For each parameter, it looks for the parameter name as a whole
  // word (\b for word boundaries) Allows any characters between parameters
  // except closing parenthesis
  const std::regex message_constructor_regex(
      R"(Message::Message\s*\()"
      R"([^)]*\bname\b[^,)]*[,)])"
      R"([^)]*\bflags\b[^,)]*[,)])"
      R"([^)]*\bpayload_size\b[^,)]*[,)])"
      R"([^)]*\bpayload_interface_id_count\b[^,)]*[,)])"
      R"([^)]*\bcreate_message_flags\b[^,)]*[,)])"
      R"([^)]*\bhandles\b[^,)]*[,)])"
      R"([^)]*\bestimated_payload_size\b[^)]*\))");

  utils::ExecuteCommand(&g_debug, "!StepIntoFunction message", true);
  debug_context_guard.RestoreIfChanged();

  // Step into the Message::Message constructor which
  // can be used for setting the flag.
  int steps = 0;
  while (steps < 5) {
    std::vector<std::string> call_stack =
        utils::GetTopOfCallStack(&g_debug, 5, false);
    if (!utils::ContainsCI(call_stack[0], "Message::Message")) {
      DERROR("Error: Failed to step into Message::Message constructor.\n");
    }

    if (std::regex_search(call_stack[0], message_constructor_regex)) {
      break;
    }

    g_debug.control->SetExecutionStatus(DEBUG_STATUS_STEP_INTO);
    g_debug.control->WaitForEvent(0, INFINITE);
    debug_context_guard.RestoreIfChanged();
    steps++;
  }

  if (steps >= 5) {
    DERROR(
        "Error: Failed to step into Message::Message constructor after 5 attempts.\n");
    return;
  }

  // At this point, we should be inside the correct Message::Message
  // constructor. Set the flag in the message and continiue execution.
  g_debug.control->SetExecutionStatus(DEBUG_STATUS_STEP_OVER);
  g_debug.control->WaitForEvent(0, INFINITE);
  debug_context_guard.RestoreIfChanged();

  utils::ExecuteCommand(&g_debug, "dx flags = flags | (1 << 29)", true);
  g_debug.control->SetExecutionStatus(DEBUG_STATUS_GO);
}

void PatchModule(const std::string& module_name) {
  // Get the current process ID
  ULONG process_id = 0;
  HRESULT hr = g_debug.system_objects->GetCurrentProcessSystemId(&process_id);
  if (FAILED(hr)) {
    DERROR("Failed to get current process ID\n");
    return;
  }

  // Skip if the patch was already applied
  // for this process_id + module_name
  for (const auto& hook_instance : g_hook_instances) {
    if (hook_instance.process_id == process_id &&
        hook_instance.module_name == module_name) {
      DOUT("Hook already installed for process %u, module %s\n", process_id,
           module_name.c_str());
      return;
    }
  }

  std::string symbol = utils::RemoveFileExtension(module_name) + "!" +
                       kHandleValidatedMessageSymbol;

  // Get the address of the target function
  ULONG64 target_func = 0;
  hr = g_debug.symbols->GetOffsetByName(symbol.c_str(), &target_func);
  if (FAILED(hr)) {
    DOUT("Failed to find HandleValidatedMessage function\n");
    return;
  }

  // Find the matching HookDefinition by checking signatures
  const HookDefinition* matching_hook_def = nullptr;
  for (const auto& hook_def : g_hook_definitions) {
    if (hook_def->CheckSignature(target_func)) {
      matching_hook_def = hook_def;
      DOUT("Function signature matches known pattern. Applying the hook...\n");
      break;
    }
  }

  if (!matching_hook_def) {
    DERROR(
        "Function signature check failed. No matching hook patterns found.\n");
    DERROR(
        "The function may have changed or this is an unsupported version.\n");
    return;
  }

  HookInstance hook_instance;
  hook_instance.process_id = process_id;
  hook_instance.hook_definition = matching_hook_def;
  hook_instance.module_name = module_name;
  hook_instance.target_address = target_func;

  if (!matching_hook_def->ApplyHook(hook_instance)) {
    DERROR("Failed to apply hook for %s\n", module_name.c_str());
    return;
  }

  // Add to the active hooks list
  g_hook_instances.push_back(hook_instance);

  DOUT("Hook [%s] installed successfully!\n",
       matching_hook_def->GetName().c_str());
  DOUT("Original function: %p\n", target_func);
  DOUT("Hook code at: %p\n", hook_instance.hook_address);
  DOUT("Jump target: %p\n", hook_instance.jump_target);
  DOUT("Process ID: %u (0x%X)\n", process_id, process_id);
  DOUT("Active hooks count: %u\n", g_hook_instances.size());
}

HRESULT CALLBACK DebugExtensionInitializeInternal(PULONG version,
                                                  PULONG flags) {
  *version = DEBUG_EXTENSION_VERSION(1, 0);
  *flags = 0;
  return utils::InitializeDebugInterfaces(&g_debug);
}

HRESULT CALLBACK DebugExtensionUninitializeInternal() {
  // Clean up the event handler
  if (g_event_callbacks) {
    g_debug.client->SetEventCallbacks(nullptr);
    g_event_callbacks->Release();
    g_event_callbacks = nullptr;
  }

  // Clean up the hook definitions
  for (auto* hook_def : g_hook_definitions) {
    delete hook_def;
  }

  return utils::UninitializeDebugInterfaces(&g_debug);
}

HRESULT CALLBACK EnableStepThroughMojoInternal(IDebugClient* client,
                                               const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "EnableStepThroughMojo - Enables stepping through Mojo messages by patching\n"
        "                        mojo::InterfaceEndpointClient::HandleValidatedMessage.\n"
        "                        When a message with bit 29 set in its flags is detected,\n"
        "                        the debugger will break and step through the handler.\n\n"
        "Usage: !EnableStepThroughMojo [module_name1] [module_name2] ...\n\n"
        "  [module_name] - Optional module names to patch (default: chrome.dll)\n\n"
        "Examples:\n"
        "  !EnableStepThroughMojo                    - Patch chrome.dll in all processes\n"
        "  !EnableStepThroughMojo chrome             - Patch chrome.dll explicitly\n"
        "  !EnableStepThroughMojo content            - Patch content.dll\n"
        "  !EnableStepThroughMojo 'chrome content'   - Patch both chrome.dll and content.dll\n"
        "Notes:\n"
        "  - The extension will automatically patch modules as they load\n"
        "  - Hooks are process-specific and persist for the lifetime of the module.\n"
        "  - This does not retroactively apply the hooks. The hooks are only\n"
        "    applied to modules that are loaded after this command is run.\n");
    return S_OK;
  }

  // Parse arguments to get module names
  std::vector<std::string> module_names;
  if (args && *args) {
    module_names = utils::ParseCommandLine(args);

    // Add .dll extension if not present
    for (auto& module_name : module_names) {
      if (!module_name.ends_with(".dll")) {
        module_name += ".dll";
      }

      g_modules.push_back(module_name);
    }
  }

  // If no modules specified, default to chrome.dll
  if (module_names.empty()) {
    g_modules.push_back("chrome.dll");
  }

  // Register event callbacks if not already done
  if (!g_event_callbacks) {
    g_event_callbacks = new EventCallbacks();
    HRESULT hr = g_debug.client->SetEventCallbacks(g_event_callbacks);
    if (FAILED(hr)) {
      g_event_callbacks->Release();
      g_event_callbacks = nullptr;
      DERROR("Failed to set event callbacks: 0x%08X\n", hr);
      return hr;
    }
  }

  return S_OK;
}

HRESULT CALLBACK ListStepThroughMojoHooksInternal(IDebugClient* client,
                                                  const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "ListStepThroughMojoHooks - Lists all active Mojo hooks and watched modules\n\n"
        "Usage: !ListStepThroughMojoHooks\n\n"
        "This command displays:\n"
        "  - Active hooks with their module names, process IDs, and addresses\n"
        "  - Modules being watched for automatic hooking when loaded\n\n"
        "Example:\n"
        "  !ListStepThroughMojoHooks\n\n"
        "See also:\n"
        "  !EnableStepThroughMojo  - Enable hooks for specific modules\n"
        "  !DisableStepThroughMojo - Remove hooks from modules\n");
    return S_OK;
  }

  if (g_hook_instances.empty()) {
    DOUT("No active Mojo hooks installed\n");
  } else {
    DOUT("Active Mojo hooks:\n");
    for (size_t i = 0; i < g_hook_instances.size(); i++) {
      const auto& hook = g_hook_instances[i];
      DOUT(
          "  %u) Module: %s, Process: %u (0x%X), Hook: %p, Int3: %p, HookName: %s\n",
          i, hook.module_name.c_str(), hook.process_id, hook.process_id,
          hook.hook_address, hook.int3_address,
          hook.hook_definition->GetName().c_str());
    }
  }

  if (!g_modules.empty()) {
    DOUT("\nModules being watched for loading:\n");
    for (const auto& module : g_modules) {
      DOUT("  - %s\n", module.c_str());
    }
  }

  return S_OK;
}

HRESULT CALLBACK StepThroughMojoInternal(IDebugClient* client,
                                         const char* args) {
  if (args && args[0] == '?' && args[1] == '\0') {
    DOUT(
        "StepThroughMojo - Starts stepping through the current Mojo call\n\n"
        "Usage: !StepThroughMojo\n\n"
        "This command initiates stepping through the current Mojo message handler.\n"
        "It should be used when execution is paused at a Mojo-related breakpoint\n"
        "before the Message instance is created (from inside a *.mojom.cc file).\n\n"
        "Example:\n"
        "  !StepThroughMojo\n\n"
        "See also:\n"
        "  !EnableStepThroughMojo  - Enable automatic breaking on Mojo messages\n"
        "  !ListStepThroughMojoHooks - List active hooks and watched modules\n");
    return S_OK;
  }

  if (g_modules.empty() || g_hook_instances.empty()) {
    DERROR("No Mojo hooks are enabled.\n");
    return E_FAIL;
  }

  StepIntoMessageAndSetFlag();
  return S_OK;
}

extern "C" {
__declspec(dllexport) HRESULT CALLBACK DebugExtensionInitialize(PULONG version,
                                                                PULONG flags) {
  return DebugExtensionInitializeInternal(version, flags);
}

__declspec(dllexport) HRESULT CALLBACK DebugExtensionUninitialize(void) {
  return DebugExtensionUninitializeInternal();
}

__declspec(dllexport) HRESULT CALLBACK
EnableStepThroughMojo(IDebugClient* client, PCSTR args) {
  return EnableStepThroughMojoInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK
ListStepThroughMojoHooks(IDebugClient* client, PCSTR args) {
  return ListStepThroughMojoHooksInternal(client, args);
}

__declspec(dllexport) HRESULT CALLBACK StepThroughMojo(IDebugClient* client,
                                                       PCSTR args) {
  return StepThroughMojoInternal(client, args);
}
}
