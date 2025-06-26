# latop.sys — Windows Keyboard Latency Optimizer

**Author:** [Amr Hamail]  
**Type:** Windows Kernel-Mode WDM Filter Driver  
**Purpose:** Reduce keyboard input latency by dynamically elevating thread priority and pinning threads to specific CPU cores.  
**Status:** Active Development

---

## ⚡ Overview

`latop.sys` is a handcrafted Windows kernel filter driver designed to reduce input lag at the system level. It intercepts keyboard read IRPs and temporarily boosts the processing thread’s priority while pinning it to a single CPU core — reducing context switching and speeding up IRP completion.

Built entirely on the Windows Driver Model (WDM), `latop.sys` provides complete control over IRP propagation, thread scheduling, and low-level latency behavior.

Every design decision — from how memory is allocated to how the IRP stack is copied — was made deliberately, from scratch, to achieve minimal input-to-response delay.

---

## 🧠 Technical Highlights

- **Thread Latency Optimization**  
  Uses `\texttt{KeSetPriorityThread}` to boost IRQL-handling threads to high priority and `\texttt{KeSetSystemAffinityThreadEx}` to lock them to a CPU core during IRP servicing.

- **IRP Lifecycle Management**  
  Intercepts and propagates IRPs using `\texttt{IoCopyCurrentIrpStackLocationToNext}`, `\texttt{IoCallDriver}`, and stack-aware completion via `\texttt{IoSetCompletionRoutine}`.

- **Safe Context Handling**  
  Allocates thread-state data in non-paged memory (priority, affinity, thread object) and safely restores it post-completion, even during edge-case interruptions.

- **Stack Integrity & Modularity**  
  Designed as a class filter driver with full support for passthrough logic, stack-safe detachment, and minimal disruption to device behavior.

---

## 🚧 Limitations

- **No symbolic link or user-mode interface** — all logic operates purely at kernel level.
- **Currently supports only keyboard class stack (kbdclass.sys)** — future HID/USB-level support planned.
- **Test environments only** — use in production environments may lead to instability.

---

## 🛠️ Build Instructions

1. Open in Visual Studio with WDK installed.
2. Build in **x64 Release** mode.
3. Load using test-signing or via tools like OSR Loader.

---

## 🔭 Future Enhancements

- IRP batching and coalescing to further reduce latency bursts
- URB-level optimization targeting USB polling delays
- Integration with `10xclick.sys` for full input stack acceleration.
- Plan to use 10xclick.sys as base to develop a driver for N-key rolling for USB keyboards. It is a forgotten feature of many PS/2 Keyboards. 

---

## 🙇 A Word from the Author

This driver is the product of curiosity and love for Windows internals — not corporate demand. I built this project line by line because I wanted to understand what happens *between* a keystroke and the software it reaches.  

---

## 📜 License

MIT License  

Copyright (c) 2025 [Amr Hamail]

Permission is hereby granted, free of charge, to any person obtaining a copy  
of this software and associated documentation files (the "Software"), to deal  
in the Software without restriction, including without limitation the rights  
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell  
copies of the Software, and to permit persons to whom the Software is  
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all  
copies or substantial portions of the Software.

**THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,**  
**INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.**  
**IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,**  
**WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.**

---

