---
name: arduino-firmware-building
description: >-
  Use this skill when the user wants to design, build, upload and test firmware for any
  Arduino-compatible board end to end with arduino-cli: identifies the board and FQBN via
  cli search, requires user-provided board/library documentation, validates requirements,
  writes plan and comprehensive test spec (including edge cases, boundary limits, negative
  inputs, and fault resilience), stops for user review & approval, implements and compiles via
  subagent, uploads with user-assisted bootloader/reset steps, captures serial test logs,
  fixes failures (max 3 loops) and coordinates human-only tests.
---

# Arduino Firmware Building

Autonomous build / flash / test loop using `arduino-cli`, serial logs and a human assistant for steps the agent cannot do. The skill is completely board-agnostic: everything board- or library-specific comes strictly from user-provided documentation, never from assumptions. **Input requirement:** the user must provide a documentation folder (or specify its path) containing the board specification (pins, memory, peripherals, upload/bootloader procedure, board options) and library API documentation. **Sketch folder:** when the skill is invoked, create a new sketch folder in the current working directory (a short snake_case name derived from the request, not already existing; the main `.ino` must have the same name as the folder) and work only inside it, unless the user names another location.

`<skill_dir>` = the folder containing this SKILL.md.

## Phase 0: Environment check and board identification (once)
1. **Toolchain check and board identification:**
   - Check `arduino-cli version` and installed cores (`arduino-cli core list`).
   - **Board Name & FQBN Search:**
     - If the user did not provide a specific board name in their request, ask them for the board name.
     - Run `arduino-cli board search <name provided by user>` to search the board package index. The output displays the matching **Board Name**, **FQBN** (Fully Qualified Board Name), and **Platform ID**.
     - If no matching board is found, ask the user again for the board name (may ask several times to clarify, check alternative names, or fix typos).
     - If the board is still not found after multiple attempts, stop and explain to the user that the board could not be found (suggest verifying the name or checking additional board manager URLs in `arduino-cli.yaml`).
     - Once the board is identified, record its exact FQBN. If its Platform ID (core) is not listed in `arduino-cli core list`, install it: `arduino-cli core install <platform_id>`.
2. **Python virtual environment:** Run `python <skill_dir>/scripts/setup_env.py` with the system Python (idempotent: creates `<skill_dir>/.venv` if missing and installs [requirements.txt](./scripts/requirements.txt)). The last printed line is the venv interpreter, normally `<skill_dir>/.venv/bin/python` (or `<skill_dir>/.venv/Scripts/python.exe` on Windows); call it `$VPY`. **Run every skill Python script with `$VPY`, never the system Python, and never `pip install` outside the venv.** To add a package, append it to requirements.txt and rerun setup_env.py.

## Phase 1: Docs and requirements
### 1a. Locate and read the user-provided docs
The skill requires an input documentation folder containing board and library specifications. Check if the user specified a path in their prompt, or look for a documentation folder in the current workspace (e.g. `docs/`, `references/docs/`, or a user-provided directory).
- **If no docs folder was specified, or the folder is empty / has no usable docs: stop immediately and ask the user to provide the documentation path or folder.** Tell the user what to provide:
  1. Board/MCU documentation: pin mappings, peripherals, display/button wiring, upload/bootloader procedure, and board configuration options.
  2. Library documentation: API docs, key classes/functions, and minimal examples for every library to be used. **Recommend markdown docs** (reading whole library source trees wastes context; raw source/headers are accepted but read selectively).
- Once located, list the files, read markdown docs fully and source/header files selectively (headers, examples, grep for needed symbols). Build a short capability summary: board facts, available libraries and APIs, known limits.
- If the documented libraries are not installed, install them using `arduino-cli lib install <library_name>`.

### 1b. Requirements
Take the user's requirement and **validate it against the docs**: is every needed feature supported by the board and the documented libraries? List unsupported or undocumented items explicitly and propose options (different approach, other library, ask user to add docs). **Ask about every unclear point** (behavior, hardware, connectivity, memory, power, timing, what counts as success). Proactively probe for edge cases, boundary limits, potential failure modes (e.g. disconnected sensors, communication dropouts, buffer limits, power glitches), and recovery expectations. Ask how the firmware should respond when inputs are out of bounds or hardware does not respond. Repeat until nothing is ambiguous. Summarize the confirmed requirements and get approval.

## Phase 2: Plan and test spec
Create from templates in [resources/](./resources/):
- `PLAN.md` ([template](./resources/PLAN.template.md)): Must be sufficiently detailed for thorough human review and sign-off. Must include:
  1. **Project directory structure:** ASCII tree diagram showing all planned sketch files, headers, test harness files, and log directories.
  2. **Source file breakdown:** Explicitly detail each file's purpose, public APIs/functions, responsibilities, and dependencies.
  3. **Architecture diagram (Mermaid):** Visual diagram showing module interactions, hardware/peripheral connections, third-party libraries, and test harness integration. **Mermaid syntax rules to prevent parse errors:** Always enclose node labels containing parentheses, brackets, or special characters in double quotes (e.g. `NODE["Label (info)"]`). Never use unquoted parentheses, brackets, or quotes inside edge labels `|...|` (e.g. `-->|M5.update()|` fails with `got 'PS'`); edge labels must either omit parentheses (e.g. `-->|Calls update|`) or be enclosed in double quotes (e.g. `-->|"M5.update()"|`).
  4. **Target board configuration:** Resolved `<FQBN>[:options]` (inspect options via `arduino-cli board details -b <FQBN>`), serial baud rate, and upload/reset procedures.
  5. **Work packages:** Clear, ordered task breakdown for the implementation agent.
  6. **Test Strategy & Edge-Case Verification:** Explicitly map failure modes, boundary conditions, and hardware edge cases to specific test IDs.
- `TESTS.md` ([template](./resources/TESTS.template.md)): **Strict requirement: Never specify only basic happy-path tests.** Basic tests easily pass while masking critical firmware bugs. Every requirement must be accompanied by edge-case and negative tests across mandatory categories:
  - **Nominal / Functional (`NOM`):** Standard operational checks with expected valid parameters.
  - **Boundary & Extremes (`BND`):** Min/max limits, zero/empty values, buffer limits, string truncation, and overflow handling.
  - **Negative & Invalid Input (`NEG`):** `NULL` pointers, out-of-range parameters, invalid commands (graceful error return without crashing).
  - **Fault Simulation & Hardware Resilience (`FLT`):** Communication timeouts, disconnected sensor / I2C NACK fallback, non-blocking execution (firmware must NEVER block in unbounded `while` loops waiting for hardware).
  - **State Machine Lifecycle (`STA`):** Re-initialization idempotency (calling `begin()` twice), uninitialized operations, state recovery.
  - **Timing & Asynchrony (`TIM`):** Rapid pulse chatter, debounce boundaries, `millis()` wraparound safety.
  - **Human Edge Cases (`HUM`):** Display boundary rendering with maximum length strings, visual fault alert indicators on error.
  Mark each test `auto` (verifiable from the serial log) or `human` (visual/audio/physical). Make as many tests auto as possible.
- Firmware must be **designed for testing**: copy [test_config.h](./resources/test_config.h), [test_harness.h](./resources/test_harness.h), [tests.h](./resources/tests.h) and [tests.cpp](./resources/tests.cpp) (template) into the sketch; self-tests run once at boot under `TEST_MODE`, store their results in global variables, and the firmware dumps them over serial when the host sends `r`. Production code stays free of test code. `TEST_MODE` is defined in `test_config.h` (source, not a compiler flag), so every file must include `test_config.h` (directly or via `tests.h`/`test_harness.h`) before testing `#ifdef TEST_MODE`. Configure `TEST_MAX_RECORDS` in `test_config.h` to match or exceed total planned `T##` + `H##` tests so no test results are dropped. The log format is fixed (see harness header); never invent other formats. Debug prints may exist but only `[TEST]` lines are judged.

### Checkpoint: Review and Approval (STOP)
At the end of Phase 2, **stop and present `PLAN.md` and `TESTS.md` to the user for review**. Do not proceed to implementation yet.
- Ask the user to review the plan, directory layout, architecture diagram, board FQBN/options, and test specifications.
- **Check for edge-case coverage:** Verify that `TESTS.md` includes boundary conditions, negative inputs, and fault resilience tests. If the test plan contains only basic happy paths, reject or expand it before presenting to the user.
- If the user provides feedback or requests revisions: update `PLAN.md` and/or `TESTS.md` accordingly, summarize the revisions, and wait for confirmation.
- Only proceed to Phase 3 once the user explicitly approves the plan and test spec.

## Phase 3: Implementation & Compilation (subagent)
Delegate implementation and compilation to a subagent with: PLAN.md, TESTS.md, the harness files, the relevant doc files, and the following rules:
1. **Implement:** Implement exactly according to PLAN.md and TESTS.md. Use only APIs found in the docs (or verified in library headers). Apply defensive embedded programming: guard against `NULL` pointers, clamp inputs to boundary ranges, and use non-blocking timeouts with `millis()` rather than bare `while()` loops on hardware registers or communication buses. Every test ID in TESTS.md (both nominal and edge-case) must be reported through the harness in `tests.cpp`. Do not upload or touch hardware.
2. **Compile:** Compile using `arduino-cli compile -b <FQBN>[:options] <sketch>` with the resolved `<FQBN>[:options]` from Phase 2. `TEST_MODE` is enabled by `#define TEST_MODE 1` in the sketch's `test_config.h`, not by a compiler flag, so the build command is identical across all boards.
3. **Iterate to clean compile:** If compilation fails, fix source errors and recompile. Implementation is complete **only when the sketch compiles with zero errors under `TEST_MODE`**.
*(If subagents are unavailable, implement and compile inline. Fix compile errors iteratively; compile fixes do not count as test loops, but stop and ask the user if compile fails 10 consecutive times.)* Review the result against the plan.

## Phase 4: Upload (user-assisted when needed)
See [upload-and-ports.md](./references/upload-and-ports.md).
1. **Download/bootloader mode (if required):** If the board docs require manual entry into bootloader/download mode (e.g. holding BOOT button, pressing RESET, double-tapping RESET), or if upload fails to connect, ask the user to enter download mode and wait for their confirmation before uploading.
2. **Upload:** Run `arduino-cli board list` to find the port, then upload:
   `arduino-cli upload -b <FQBN> -p <PORT> <sketch>`.
3. **Reset to run firmware (ALWAYS):** We do not assume which boards auto-reset after uploading. Therefore, **always instruct the user to reset the board** (e.g., press the physical RESET button or power-cycle according to board docs) so the firmware starts running cleanly. Even if a board has auto-reset capability, performing a manual reset is harmless and guarantees the firmware is executing. **Wait for user confirmation before proceeding.**
4. **Identify serial port:** Run `$VPY scripts/capture_serial.py --list`. The port may have changed from the bootloader/upload port; use the active port running the firmware.

## Phase 5: Capture and analyze
1. `$VPY scripts/capture_serial.py --port <PORT> --baud <BAUD> --out logs/run<N>.log --timeout 90 --send r` (`r` makes the firmware dump the results its boot-time tests stored in global variables, so the log is complete even if boot output was missed; baud must match the firmware's `Serial.begin`).
2. `$VPY scripts/parse_results.py logs/run<N>.log --expect <T ids from TESTS.md> --json logs/run<N>.json`. Do not parse logs by hand. Generic crash patterns are built in; if the board docs list platform-specific crash signatures, add them with `--crash-pattern <regex>` (repeatable). Exit codes: 0 all pass, 1 failures or crash, 2 incomplete (no DONE or missing ids).
3. Exit 2: check port/timeouts/boot loop; read the raw log to diagnose (crash output, wrong port or baud, firmware stuck).

## Phase 6: Fix loop (max 3 iterations)
If any auto test fails: analyze the log, fix the code (subagent or inline), recompile until clean, and repeat Phase 4 to 5. **Stop after 3 loops** (and stop early if the same failure repeats unchanged), then report what failed, hypotheses and what you tried.

## Phase 7: Human tests
For each `H##` test, give the user one message in this exact form, then wait:

```
HUMAN TEST H01: <title>
Do:      <exact action>
Observe: <exactly what to look/listen for>
Reply:   H01: PASS  or  H01: FAIL - <what you saw/heard>   (photo/video/audio path welcome)
```

Batch all human tests in a single message in firmware order, triggering the firmware action via `--send r` (dumps stored results) or the input specified in the test. Photos can be read by the agent from a file path; sound must be described in words for now. Record replies in `logs/human_results.md`. A FAIL enters the fix loop like an auto failure (counts toward the 3 loops).

## Phase 8: Final report
Summarize: requirements covered, auto results (pass/fail per ID), human results, loops used, files changed, doc gaps found, open issues. Optionally rebuild for production (comment out `#define TEST_MODE 1` in `test_config.h`) and say so.

## Notes
- Always re-list ports after any mode change; never reuse an old port.
- Keep all logs in `<sketch>/logs/`.
- Missing or insufficient docs are a blocker for the affected feature: ask, do not guess APIs.
