# Copilot Instructions for ECE 306 Project 10_b

## Project Overview
- This is an embedded C project for the MSP430FR2355 microcontroller, focused on autonomous robot control (line following, motor control, sensor integration).
- Major components are split by hardware function: wheels (motor logic), PWM (motor speed/direction), ADC (sensor input), DAC (analog output), LCD (display), and supporting modules (timers, interrupts, calibration).

## Architecture & Data Flow
- `main.c` initializes the system and runs the main loop.
- Motor control is handled by `wheels.c` (high-level logic) and `PWM.c` (low-level PWM, direction safety).
- Sensor readings (line sensors, etc.) are managed via `ADC.c` and used in control logic.
- Display updates use `LCD.h` and `display_line` globals.
- Timers and interrupts (`timers_*.c`, `interrupts_*.c`) are used for precise timing and asynchronous events.
- Calibration routines are in `calibration.c` and must be run before autonomous features.

## Key Patterns & Conventions
- All hardware access is abstracted via macros (see `macros.h`) and functions.
- Motor direction changes are protected by a 500ms enforced delay (`PWM.c:Apply_Direction_Change_Delay`). Use only high-level movement functions (e.g., `PWM_FORWARD`, `PWM_REVERSE`) unless you are certain immediate direction change is safe.
- Display lines are updated by writing to `display_line[4][11]` and setting `display_changed = 1`.
- State machines are used for autonomous routines (see `wheels.c:Line_Follow_Process`).
- Use `Wheels_Safe_Stop()` for emergency stops.
- Calibration status is checked via `calibration_complete` before running autonomous routines.
- ISR  functions should be kept short; defer processing to main loop flags. Functions should not be called directly from ISRs unless they are specifically designed for that purpose.

## Developer Workflows
- Build system CCS (TI Code Composer Studio). See `Debug/makefile` for build rules.
- No automated tests are present; debugging is done via display output and hardware observation.
- To add new features, follow the modular structure: create new `*.c`/`*.h` pairs and update `main.c` as needed.
- Use the `backup/` folder for manual code snapshots; do not edit these directly.

## Integration Points
- All cross-module communication is via global variables or function calls; no dynamic memory or OS features are used.
- External dependencies: MSP430 hardware, TI toolchain.

## Examples
- To start line following: call `Line_Follow_Start_Autonomous()` after calibration.
- To stop all motors safely: call `Wheels_Safe_Stop()` and set DAC to `DAC_MOTOR_OFF`.

## File References
- `wheels.c`, `PWM.c`, `ADC.c`, `calibration.c`, `LCD.h`, `macros.h`, `main.c`, `timers_*.c`, `interrupts_*.c`

---
For any unclear conventions or missing documentation, ask for clarification or check the relevant source file.
