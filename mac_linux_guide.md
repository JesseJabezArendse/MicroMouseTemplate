# UCT Micromouse: Mac & Linux Standalone Setup

The official MATLAB Hardware Support Package for STM32 is only available on Windows. However, Mac and Linux users can easily compile and flash their code using a standard open-source toolchain!

## 1. Prerequisites
You need the ARM cross-compiler and GNU Make installed on your system.

**For Mac (Using Homebrew):**
```bash
brew install arm-none-eabi-gcc make
```

**For Linux (Ubuntu/Debian):**
```bash
sudo apt update
sudo apt install gcc-arm-none-eabi make
```

## 2. The Development Workflow
Because you are bypassing the Windows STM32 addon, your deployment process is split into two steps: **Generate** (in MATLAB) and **Flash** (in Terminal).

1. **Generate the Code:** 
   * Open your `StudentTemplate.slx` (or `MicroMouse_Deploy.slx`) in Simulink.
   * **If you get an error saying the STM32L4xx support package is not installed:**
     1. Open the **Configuration Parameters** (press **Cmd + E** or click the gear icon).
     2. Under the **Hardware Implementation** pane, set **Hardware board** to **`None`**.
     3. Under the **Code Generation** pane, ensure the **System target file** is set to `ert.tlc` (Embedded Coder) or `grt.tlc` (Simulink Coder).
     4. On the same **Code Generation** pane, check the box for **`Generate code only`**.
     5. Click **Apply** and close the configuration.
   * Press **Cmd + B** (or click the **Build** button). 
   * *Note: Do NOT click "Deploy to Hardware" as this will attempt to invoke the missing Windows addon and crash.* 
   * Simulink will generate the raw C-code and dump it into the build directory (typically `MicroMouseProgramming_Code` or the target model's `_ert_rtw` folder).

2. **Flash the Mouse:**
   * Open your Terminal and navigate to the root directory of this repository (where this guide is located).
   * Plug your Micromouse into your computer via USB.
   * Run the deployment script:
     `./deploy_mac_linux.sh`
   * The script will compile your generated code using the ARM toolchain and automatically copy the firmware to the mouse!