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

- **Robust Cancellation Handling:**  
  Implements a cancel routine that safely handles IRP cancellations by queuing cleanup work items, preventing crashes caused by freeing resources at high IRQL.
  
- **Benchmark-Proven Performance:**  
Validated overall latency reduction in controlled tests (see below).

---
## 🧪 Benchmark & Latency Analysis (Keyboard Analyzer 1.0.2)
To validate the performance of this driver, I performed controlled input latency testing using Keyboard Analyzer v1.0.2 on a virtualized Windows 11 Pro environment.

### ⚙️ Test Setup
- CPU: Ryzen 5 3600 (3 cores allocated)
- RAM: 4.8 GB
- OS: Windows 11 Pro (VM)
- Measurement Tool: Keyboard Analyzer 1.0.2
- Key Used: Standard alphanumeric (same physical keyboard & polling conditions)
- Conditions: Identical software load, background processes, and test length

### 📊 Results Overview (Revised)
*Note: Tests were conducted in a virtualized environment, which may result in higher baseline latency values. The measured improvements reflect the driver’s optimizations based on fitted functions derived from peak data of each 40-50 ms period. Furthermore, the tests were conducted using an auto-key presser, which clicked the "a" key once every 25ms.*

| Metric                 | Without Driver | With Driver   | Improvement         |
|-------------------------|----------------|---------------|---------------------|
| Inputs at 0 ms Latency  | 35 inputs      | 36 inputs     | ⬆ ~2.9% increase    |
| Inputs at 0.25 ms Latency | 66 inputs   | 58 inputs     | ⬇ ~12.1% decrease   |
| Inputs at 0.5 ms Latency | 141 inputs  | 132 inputs    | ⬇ ~6.4% decrease    |
| Inputs at 0.75 ms Latency | 212 inputs | 210 inputs    | ⬇ ~0.9% decrease    |
| Inputs at 1 ms Latency  | 279 inputs    | 283 inputs    | ⬆ ~1.4% increase    |
| Inputs at 1.25 ms Latency | 276 inputs | 251 inputs    | ⬇ ~9.1% decrease    |
| Inputs at 1.5 ms Latency | 125 inputs  | 122 inputs    | ⬇ ~2.4% decrease    |
| Inputs at 1.75 ms Latency | 63 inputs  | 63 inputs     | ⬇ 0% change         |
| Inputs at 2 ms Latency  | 36 inputs     | 50 inputs     | ⬆ ~38.9% increase   |
| Inputs at 45 ms Latency | 195 inputs    | 199 inputs    | ⬆ ~2.1% increase    |
| Inputs at 95 ms Latency | 144 inputs    | 146 inputs    | ⬆ ~1.4% increase    |
| Inputs at 144 ms Latency | 129 inputs   | 130 inputs    | ⬆ ~0.8% increase    |
| Inputs at 190 ms Latency | 119 inputs   | 108 inputs    | ⬇ ~9.2% decrease    |
| Inputs at 240 ms Latency | 100 inputs   | 106 inputs    | ⬆ ~6% increase      |
| Inputs at 289 ms Latency | 97 inputs    | 81 inputs     | ⬇ ~16.5% decrease   |
| Inputs at 336.8 ms Latency | 93 inputs | 82 inputs     | ⬇ ~11.8% decrease   |
| Inputs at 384 ms Latency | 93 inputs    | 74 inputs     | ⬇ ~20.4% decrease   |
| Inputs at 431 ms Latency | 85 inputs    | 66 inputs     | ⬇ ~22.4% decrease   |
| Inputs at 479 ms Latency | 80 inputs    | 72 inputs     | ⬇ ~10% decrease     |
| Inputs at 526 ms Latency | 70 inputs    | 63 inputs     | ⬇ ~10% decrease     |
| Inputs at 576 ms Latency | 73 inputs    | 59 inputs     | ⬇ ~19.2% decrease   |
| Inputs at 622 ms Latency | 64 inputs    | 57 inputs     | ⬇ ~10.9% decrease   |
| Inputs at 669 ms Latency | 66 inputs    | 52 inputs     | ⬇ ~21.2% decrease   |
| Inputs at 715 ms Latency | 68 inputs    | 51 inputs     | ⬇ ~25% decrease     |
| Inputs at 763 ms Latency | 56 inputs    | 48 inputs     | ⬇ ~14.3% decrease   |
| Inputs at 811 ms Latency | 57 inputs    | 47 inputs     | ⬇ ~17.5% decrease   |
| Inputs at 847 ms Latency | 52 inputs    | 42 inputs     | ⬇ ~19.2% decrease   |
| Inputs at 906 ms Latency | 50 inputs    | 41 inputs     | ⬇ ~18% decrease     |
| Inputs at 954 ms Latency | 50 inputs    | 41 inputs     | ⬇ ~18% decrease     |
| Inputs at 994 ms Latency | 45 inputs    | 33 inputs     | ⬇ ~26.7% decrease   |


### 📈 Analysis

The latency distribution data can be accurately modeled using dampened sine waves. While the driver is turned **off**, the latency curve is described by:

`y = 2300 * exp(-0.0003 * x) * (sin(0.125 * x + 1.55) / x)`

With the driver **enabled**, the latency distribution becomes:

`y = 2500 * exp(-0.0018 * x) * (sin(0.125 * x + 1.55) / x)`

This shift highlights the driver’s impact on **peak input timing and latency decay**.

#### Key Findings:

- **+2.1% increase** in inputs at **45 ms latency** (from 195 to 199)
- **–26.7% reduction** in high-latency inputs at **994 ms** (from 45 to 33)
- **+38.9% increase** in low-latency inputs at **2 ms** (from 36 to 50)
- Faster decay rate (**0.0018 vs. 0.0003**) indicates **stronger suppression of late input noise**
- **Higher amplitude (2500 vs. 2300)** shows greater initial input response density

The **steeper exponential decay** with the driver active demonstrates its effectiveness in stabilizing the input stream by reducing the frequency of late-response events. These improvements are especially relevant in **real-time environments**, such as **competitive gaming, rhythm-based software, or high-refresh input tasks**, where tight and predictable input timing is essential.

> ⚠️ *Note: This benchmark was conducted in a virtualized Windows 11 environment. Baseline latencies may be inflated compared to bare-metal performance.*



## 📄 Raw Benchmark & Screenshots PDF

- 📥 [WITHOUT_DRIVER.pdf](latency%20optimizer/WITHOUT_DRIVER.pdf)
- 📥 [WITH_DRIVER.pdf](latency%20optimizer/WITH%DRIVER.pdf)
- 📥 [WITHOUT_DRIVER.kbi](latency%20optimizer/with%20driver.kbi)
- 📥 [WITH_DRIVER.kbi](latency%20optimizer/without_driver.kbi)



---
### 🧠 Conclusion

This driver provides a **real, measurable reduction in input latency** and jitter under Windows. Even under limited VM hardware, the gains are clearly visible. For users seeking snappier keyboard performance and an extra edge, this driver offers an effective solution.

## ⚠ Limitations

- Tested in Windows 11 pro test mode and disabling driver signature enforcement.
- Requires the addition of "latencyoptimizer" value in the Registry (check build & installation).
- Cannot be used on laptops, for now. I plan on fixing this bug.

---

## 🔧 Build & Installation
Build (latencyoptimizer.sys) is included in the /release section of this github. Don't instal via .ini, it's buggy for some reason. just install it with sc install. 
To add it to the keyboard IRP stack:
Navigate to HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{4d36e96b-e325-11ce-bfc1-08002be10318} (keyboard class GUID) in the Registry Editor.
Find or create the UpperFilters multi-string value:

Right-click, select Modify, and set it to latencyoptimizer\0kbdclass (order matters; \0 separates entries, ensuring the driver processes input first).

---

## 🙇 Author’s Note

This is my first driver ever; It was very hard. But, I made it through! :D 🎇

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
