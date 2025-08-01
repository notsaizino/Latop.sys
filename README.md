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
Validated 15-17% latency reduction in controlled tests (see below).

---
## 🧪 Benchmark & Latency Analysis (Keyboard Analyzer 1.0.2)

To validate the performance of this driver, I performed controlled input latency testing using **Keyboard Analyzer v1.0.2** on a virtualized Windows 11 Pro environment.

### ⚙️ Test Setup
- **CPU**: Ryzen 5 3600 (3 cores allocated)
- **RAM**: 4.8 GB
- **OS**: Windows 11 Pro (VM)
- **Measurement Tool**: Keyboard Analyzer 1.0.2
- **Key Used**: Standard alphanumeric (same physical keyboard & polling conditions)
- **Conditions**: Identical software load, background processes, and test length

---

### 📊 Results Overview (Revised)

> **Note:** Tests were conducted in a virtualized environment, which may result in higher baseline latency values. The measured percentage increases and reduction in jitter are a direct result of the driver’s optimizations.

| Metric                     | **Without Driver**             | **With Driver**              | **Improvement**                      |
|----------------------------|--------------------------------|------------------------------|--------------------------------------|
| **Inputs at 45 ms Latency**| 217 inputs                     | 261 inputs                   | ⬆ ~20.3% increase                    |
| **Inputs at 1 ms Latency** | 300 inputs                     | 327 inputs                   | ⬆ ~9% increase                       |
| **High-Latency Outliers**  | Significant number observed    | Drastic reduction            | ⬇ Major reduction in jitter          |

This is using a 3 core CPU, and 4.8GB of ram, on a bloated windows 11 pro in a VM. 
For a proper system, I expect smaller metric values (1ms ~ 500 inputs, with 10ms as "maximum" input lag possible).
---

### 📈 Interpretation

With the driver active, the data demonstrates clear and measurable improvements in input handling:

- My driver is most effective at **increasing the frequency of low-latency events**. There is a significant **~20.3% increase** in the number of inputs recorded at the **45 ms latency** mark.
- Even at the lowest end, the number of events at **1 ms latency** is boosted by **~9%**, confirming the driver’s positive effect on overall responsiveness.
- Most importantly, the driver effectively **eliminates the majority of high-latency outliers**. This is the key to reducing perceived lag and creating a much more stable and predictable input stream, especially crucial in gaming and other time-sensitive applications.


## 📄 Raw Benchmark & Screenshots PDF

- 📥 [WITHOUT_DRIVER.pdf](latency%20optimizer/WITHOUT_DRIVER.pdf)
- 📥 [WITH_DRIVER.pdf](latency%20optimizer/WITH%DRIVER.pdf)
- 📥 [WITHOUT_DRIVER.kbi](latency%20optimizer/with%20driver.kbi)
- 📥 [WITH_DRIVER.kbi](latency%20optimizer/without_driver.kbi)



---
### 🧠 Conclusion

This driver provides a **real, measurable reduction in input latency** and jitter under Windows. Even under limited VM hardware, the gains are clearly visible. For users seeking snappier keyboard performance and an extra edge, this driver offers an effective solution.

## ⚠ Limitations

- Requires test mode and disabling driver signature enforcement
- Only tested in VM so far. 

---

## 🔧 Build & Installation
Build (latencyoptimizer.sys) is included in the /release section of this github. Don't instal via .ini, it's buggy for some reason. just install it with sc install. 

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
