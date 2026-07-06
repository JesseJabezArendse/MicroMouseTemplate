# MicroMouseTemplate


## Recommended: Fork This Repository

To start your own MicroMouse project, **fork this repository** on GitHub and then clone your fork:

```sh
git clone https://github.com/<your-username>/MicroMouseTemplate.git
```

This gives you full control and allows you to push your changes to your own repository. All paths in this repo are relative, so it works from any subdirectory.

## Getting Started (Initialization Tasks)

To set up your development environment for this repository, run the following initialization tasks in VS Code:

1. **Install Python Dependencies**
   - Task: `Initialization: Install Python Dependencies`
   - Installs all required Python packages from `Scripts/requirements.txt`.

2. **Setup ARM Toolchain**
   - Task: `Initialization: Setup ARM Toolchain`
   - Downloads and configures the ARM GCC toolchain locally in `Toolchains/` and updates VS Code settings automatically.

3. **Setup OpenOCD**
   - Task: `Initialization: Setup OpenOCD`
   - Downloads and configures OpenOCD locally in `Toolchains/` and updates VS Code settings automatically.

4. **Start MicroMouse Interface** (optional, for web interface development)
   - Task: `Start MicroMouse Interface`
   - Launches the Python web interface for MicroMouse.

5. **Open MicroMouse Interface** (optional)
   - Task: `Open MicroMouse Interface`
   - Opens the web interface in your browser at [http://127.0.0.1:5000](http://127.0.0.1:5000).

> **Tip:** Run these tasks from the VS Code Command Palette (`Ctrl+Shift+P` → `Tasks: Run Task`).

---

## First Time Hardware setup
After completing your hardware assembly, there are 4 demo binary files. These are for the 4 permutations for the 4 potential wiring of your motors.
Flash AlignDemoX.bin onto the processor board.
Press one of the buttons close to the IMU to activate the motors.
If your robot isnt travelling forwards, flash a different DemoX until your robot is trying to move forward. 
It should avoid obstacles on each of its sides and indicate on the LEDS of the processor board if there is an object within 200mm

This should indicate that your hardware works

## For students:
Use [`StudentTemplate.slx`](./StudentTemplate.slx) as a clean starting point for your own implementation.  
→ Build on this file when creating your own solution.

## For programming the MicroMouse:
Use [`MicroMouse_Deploy.slx`](./MicroMouse_Deploy.slx)  
→ Go to **Hardware > Build, Deploy and Start**

## To skip the long build times and run directly from your machine for an unlocked and faster experience:
Use [`MicroMouse_RapidDevelopment.slx`](./MicroMouse_RapidDevelopment.slx)  
→ Go to **Simulation > Run**

## For developing with C Code:
Open the [`MicroMouseProgramming_Code`](./MicroMouseProgramming_Code) directory as an STM32 Project in VS Code or STM32CubeIDE.

### Setting up the ARM Toolchain
Before building C code, you need to set up the ARM toolchain. A convenient VS Code task is provided:

1. Open the repository in VS Code
2. Open the Command Palette (`Ctrl+Shift+P`)
3. Run **Tasks: Run Task**
4. Select **Setup ARM Toolchain**

The task will:
- Download the latest `arm-none-eabi-gcc` from [xpack-dev-tools](https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/)
- Extract it to `./Toolchains/arm-none-eabi-gcc/` in the repository root
- Automatically configure VS Code to use this toolchain
- Remove any previous toolchain version

No manual path configuration is needed—the settings are pre-configured to use the toolchain at `./Toolchains/arm-none-eabi-gcc/bin/`.

