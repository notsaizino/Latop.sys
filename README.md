# latop.sys – Latency Optimized Keyboard Driver

Author's note: This Driver is currently untested. Use at your own risk. 

## 🧠 Overview

**`latop.sys`** is a Windows kernel-mode filter driver designed to reduce input latency from the keyboard by intervening in the IRP's (I/O Request Packet) processing path. It intercepts `IRP_MJ_READ` requests for the keyboard and temporarily boosts the thread's scheduling context (priority and affinity) during processing. It does not alter or reroute the IRP, ensuring full compatibility with the existing driver stack.


---

## ⚙️ Architecture

### ➤ Device Stack Filtering

The driver attaches itself to the **keyboard class driver** as a **filter device**. It does not replace or disable any default behavior—it merely intercepts `IRP_MJ_READ` requests meant for the keyboard, applies low-latency optimizations, and then passes them down the stack untouched.

- `DeviceAttach()` creates a filter device and attaches it to the keyboard stack using `IoAttachDeviceToDeviceStackSafe`.
- The `DEVICE_EXTENSION` structure stores the pointer to the next device below us (`LowerDeviceObject`) so we can forward IRPs properly.

### ➤ IRP Handling & Optimization

The core optimization happens in `ReadKeyboardInput()`:

1. **Boosts thread priority** with `KeSetPriorityThread` (to `HIGH_PRIORITY`).
2. **Pins the thread to CPU 0** using `KeSetSystemAffinityThreadEx`, to reduce context switching.
3. **Stores old state** (thread priority and core affinity) in a custom context structure (`MY_COMPLETION_CONTEXT`).
4. **Sets a Completion Routine** to restore the thread's original priority, to remove the affinity pin, and to free the allocated pool for `MY_COMPLETION_CONTEXT`.

The IRP then continues down the device stack. This is **non-invasive**—the keyboard still works normally, just with faster dispatch latency.

### ➤ Completion Routine

`CompletionRoutine()` restores the original thread priority and CPU affinity. It also logs its execution using `KdPrint`.

This routine does **not** alter the IRP data. It’s only concerned with thread-level control.

---

## 🧹 Clean Unload

The `latopUnload()` routine:

- Detaches the filter(s) from the device stack via `IoDetachDevice`.
- Deletes all filter device objects with `IoDeleteDevice`.

The loop ensures that **all device objects created by this driver** are cleaned up, even if more than one exists. Currently, no symbolic link is implemented (planned for phase 2).

---

## 🛑 IRP Routing

- `IRP_MJ_CREATE` and `IRP_MJ_CLOSE` are simply passed through.
- `IRP_MJ_READ` is where latency optimization happens.
- All other IRPs are currently unsupported and are not intercepted or altered.

---

## 🔐 Safety & Stability

- Thread modifications are restored *immediately* in the completion routine.
- No memory leaks—`ExAllocatePoolWithTag` always has a matching `ExFreePoolWithTag`.
- `IoSetCompletionRoutine` ensures restoration logic runs *even on IRP failure*.
- Logging is handled via `KdPrint` for debugging and visibility.

---

## 📌 Notes for the Future

Planned improvements:

- Add a **symbolic link interface** for optional user-mode interaction.
- Explore:
  - IRP Batching, and USB Polling via URB_FUNCTION_INTERRUPT_TRANSFER_
  - Real-time input logging and timestamp analysis
  - Kernel bypass optimizations
- Possibly integrate registry configuration for tuning affinity/priority.

---

## 🔧 Build & Deployment

This driver is built for **Windows 10+** using the **Windows Driver Kit (WDK)**.
