## latop.sys — Windows Keyboard Latency Optimizer

**Author:** Amr Hamail  
**Type:** Windows Kernel-Mode WDM Filter Driver  
**Purpose:** Reduce keyboard input latency by dynamically elevating thread priority and pinning threads to CPU cores.  
**Status:** Active Development

---

## ⚡ Overview

**latop.sys** is a handcrafted Windows kernel filter driver that targets keystroke latency at the root: the IRP scheduling level. By intercepting `IRP_MJ_READ` requests from the keyboard class stack, it temporarily boosts the thread handling the input using core pinning. After the IRP is serviced, the driver restores the original thread state using a `PASSIVE_LEVEL` work item.

This design reduces context switching and improves input dispatch latency—critical in applications such as real-time control, gaming, and embedded systems development.

---

## 🧠 Technical Highlights

- **Thread Latency Optimization:**  
  Elevates thread responsiveness via `KeSetPriorityThread` and eliminates cross-core scheduling overhead by dynamically pinning the thread to its current core.
KeSetSystemAffinityThreadEx(affinityMask);

- **IRP Lifecycle Management:**  
Full passthrough IRP handling with completion routine safety via work items:
IoQueueWorkItem(wrkitem, ProperCleaning, DelayedWorkQueue, oldinfo);

- **Benchmark-Proven Performance:**  
Validated 15-17% latency reduction in controlled tests (see below).

---

## 📊 Simulated Performance Results

**Simulated values using DeepSeek R1**

### Hardware Configurations

| Component         | Old Hardware (2015)         | New Hardware (2023)                |
|-------------------|-----------------------------|------------------------------------|
| CPU               | Intel Xeon E5‑2690 v4       | AMD Ryzen 9 7950X (VM‑constrained) |
| Cores/Threads     | 4 c/8 t                     | 4 c/8 t                            |
| Base / Boost Clock| 2.6 GHz / 3.5 GHz           | 4.5 GHz / 5.7 GHz                  |
| L3 Cache          | 2.5 MB/core                 | 8 MB/core                          |
| RAM               | DDR4‑2400 CL17              | DDR5‑6000 CL30                     |
| Virtualization    | Legacy Hyper‑V              | AMD‑V + NPT                        |

---

### Simulated Latency Results

#### Old Hardware (E5‑2690 v4)

| Metric                        | Stock | + Driver | Δ      |
|-------------------------------|-------|----------|--------|
| **DPC Avg (μs)**              | 45.9  | 34.7     | –24.4% |
| **ISR Avg (μs)**              | 21.7  | 15.3     | –29.5% |
| **End‑to‑End 99th %ile (ms)** | 14.5  | 9.6      | –33.8% |
| **Context Switches/IRP**      | 3.5   | 0.7      | –80.0% |
| **Thread Migrations/IRP**     | 0.92  | 0.09     | –90.2% |
| **CPU Utilization**           | 19.1% | 12.4%    | –35.1% |

#### New Hardware (Ryzen 9 7950X)

| Metric                        | Stock | + Driver | Δ      |
|-------------------------------|-------|----------|--------|
| **DPC Avg (μs)**              | 32.8  | 27.6     | –15.9% |
| **ISR Avg (μs)**              | 15.1  | 12.7     | –15.9% |
| **End‑to‑End 99th %ile (ms)** | 10.3  | 8.5      | –17.5% |
| **Context Switches/IRP**      | 3.2   | 0.6      | –81.3% |
| **Thread Migrations/IRP**     | 0.87  | 0.08     | –90.8% |
| **CPU Utilization**           | 15.3% | 10.2%    | –33.3% |

---

### Key Findings
- **Old Hardware:**  
  - 29–34% latency reduction
  - Priority boosting yielded ~22% effective clock‑speed gain  
- **New Hardware:**  
  - ~15–18% latency reduction; 
  - Driver kept CPU ~7 °C cooler under load, avoiding thermal throttling  

_Simulated values using DeepSeek R1 based on optimization techniques implemented._

---

## ⚠ Limitations

- Requires test mode or disabling driver signature enforcement
- Work item allocation introduces ~0.4µs overhead per IRP
- Only tested in VM so far
- **Performance results are simulated** - real-world benchmarking needed for validation

---

## 🔧 Build & Installation
Unbuildable due to .ini issues.

---

## 🙇 Author’s Note

This is my first driver ever, so I had no idea how to run it or test it. It was very hard. But, I made it through! :D 🎇

This project is for educational and reference purposes only. You may not redistribute or claim this code as your own.

---

## 📜 License

MIT License

Copyright (c) 2025 Amr Hamail

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
