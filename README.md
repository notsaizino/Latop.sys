# 🎹 `latop.sys` — Latency-Optimized Keyboard Driver

> **⚠️ Author's Note:** This driver is currently untested. Use at your own risk. It may crash your system or behave unpredictably. Test only in a controlled environment (e.g., VM or dedicated test bench).

---

## 🧠 Overview

**`latop.sys`** is a Windows **kernel-mode filter driver** crafted to reduce input latency by accelerating the processing of `IRP_MJ_READ` requests for keyboard input.

Rather than altering or rerouting IRPs, this driver:
- Boosts the thread priority temporarily
- Pins the IRP-handling thread to a specific CPU core
- Passes the IRP unmodified down the stack

The result: lower input-to-processing delay with full compatibility.

---

## ⚙️ Architecture

### 🔩 Device Stack Filtering

`latop.sys` attaches to the keyboard class device stack as a **filter driver**. It does **not** replace or modify the behavior of the default drivers.

- `DeviceAttach()` creates a filter device and attaches it using `IoAttachDeviceToDeviceStackSafe`.
- The filter’s `DEVICE_EXTENSION` holds a pointer to the next device (`LowerDeviceObject`) to enable proper IRP forwarding.

> **💡 Note:** This is a *passive* filter—it doesn’t block or modify IRP content.

---

### ⚡ IRP Handling & Optimization

Inside `ReadKeyboardInput()`:

- 🎚️ Increases the IRP-handling thread’s priority using `KeSetPriorityThread`.
- 🎯 Pins the thread to **CPU Core 0** using `KeSetSystemAffinityThreadEx` to reduce context switching.
- 💾 Saves the original thread state in a custom structure: `MY_COMPLETION_CONTEXT`.
- 🔁 Uses `IoSetCompletionRoutine()` to revert the thread to its normal state after IRP handling.
- ➡️ Forwards the IRP using `IoCallDriver()`.

This logic is invisible to user applications—the IRP reaches its destination, just faster.

---

### 🔄 Completion Routine

In `CompletionRoutine()`:

- Restores thread priority and core affinity via:
  - `KeSetPriorityThread`
  - `KeRevertToUserAffinityThreadEx`
- Releases kernel thread object with `ObDereferenceObject`
- Frees memory using `ExFreePoolWithTag`
- Logs the restore action via `KdPrint`

**💡 Side-effect free:** This routine touches only scheduling state—not the IRP data itself.

---

## 🧹 Driver Unload & Cleanup

The `latopUnload()` function:

- Calls `IoDetachDevice()` to unlink the filter from the stack.
- Deletes each filter device using `IoDeleteDevice()`.
- Iterates through all `DeviceObject`s to ensure no objects are leaked.

> **Note:** No symbolic link is implemented yet. This is planned for **Phase 2**.

---

## 🛑 IRP Routing

| IRP Type        | Behavior                     |
|----------------|------------------------------|
| `IRP_MJ_CREATE` | Pass-through (no-op)         |
| `IRP_MJ_CLOSE`  | Pass-through (no-op)         |
| `IRP_MJ_READ`   | Latency optimization logic   |
| Others          | Currently ignored            |

---

## 🔐 Safety & Stability

✅ Memory safe: All allocations via `ExAllocatePoolWithTag` are freed via `ExFreePoolWithTag`  
✅ Object-safe: Threads are properly dereferenced  
✅ Robust: Completion routine guarantees restoration even on IRP failure  
✅ Transparent: Does not alter IRP payloads or prevent processing

---

## 🚀 Future Improvements

- 🔗 Implement symbolic link for user-mode control
- 🧵 IRP Batching (multiple `IRP_MJ_READ`s queued simultaneously)
- 🧬 USB Polling support via `URB_FUNCTION_INTERRUPT_TRANSFER`
- 🕐 Timestamped input logging + latency telemetry
- 🛣️ Potential HID stack bypass for lower roundtrip latency

---

## 🛠 Build & Deployment

- **Target OS:** Windows 10+
- **Toolchain:** Windows Driver Kit (WDK) + Visual Studio
- **Architecture:** x64
- **Recommended:** Test in virtual machine with kernel debugging enabled

---

## 📎 Final Thoughts

`latop.sys` is a living experiment in input latency reduction at the driver level. While it’s not production-tested, it reflects deep architectural understanding and low-level driver engineering.

---
