---
name: arduino-firmware-building
description: >-
  Use this skill when the user wants to design, build, upload and test firmware for any
  Arduino-compatible board end to end with arduino-cli: requires an input documentation folder
  containing board specifications and library documentation provided by the user, validates
  requirements against them, writes a plan and test spec, delegates implementation to a
  subagent, compiles, has the user handle manual board steps, uploads, captures serial
  test logs, fixes failures (max 3 loops) and coordinates human-only tests.
---

# Arduino Firmware Building

Autonomous build / flash / test loop using `arduino-cli`, serial logs and a human assistant for steps the agent cannot do. The skill is completely board-agnostic: everything board- or library-specific comes strictly from user-provided documentation, never from assumptions. **Input requirement:** the user must provide a documentation folder (or specify its path) containing the board specification (pins, memory, peripherals, upload/bootloader procedure, FQBN, board options) and library API documentation. **Sketch folder:** when the skill is invoked, create a new sketch folder in the current working directory (a short snake_case name derived from the request, not already existing; the main `.ino` must have the same name as the folder) and work only inside it, unless the user names another location.

`<skill_dir>` = the folder containing this SKILL.md.

## Phase 0: Environment check (once)
1. `arduino-cli version`. Check installed cores/libraries (`arduino-cli core list`, `arduino-cli lib list`); install what the docs say is needed.
2. **Python virtual environment.** Run `python <skill_dir>\scripts\setup_env.py` with the system Python (idempotent: creates `<skill_dir>\.venv` if missing and installs [requirements.txt](./scripts/requirements.txt)). The last printed line is the venv interpreter, normally `<skill_dir>\.venv\Scripts\python.exe`; call it `$VPY`. **Run every skill Python script with `$VPY`, never the system Python, and never `pip install` outside the venv.** To add a package, append it to requirements.txt and rerun setup_env.py.

## Phase 1: Docs and requirements
### 1a. Locate and read the user-provided docs
The skill requires an input documentation folder containing board and library specifications. Check if the user specified a path in their prompt, or look for a documentation folder in the current workspace (e.g. `docs/`, `references/docs/`, or a user-provided directory).
- **If no docs folder was specified, or the folder is empty / has no usable docs: stop immediately and ask the user to provide the documentation path or folder.** Tell the user what to provide:
  1. Board/MCU documentation: pin mappings, memory, peripherals, upload/bootloader procedure, FQBN, and board options.
  2. Library documentation: API docs, key classes/functions, and minimal examples for every library to be used. **Recommend markdown docs** (reading whole library source trees wastes context; raw source/headers are accepted but read selectively).
- Once located, list the files, read markdown docs fully and source/header files selectively (headers, examples, grep for needed symbols). Build a short capability summary: board facts, available libraries and APIs, known limits.

### 1b. Requirements
Take the user's requirement and **validate it against the docs**: is every needed feature supported by the board and the documented libraries? List unsupported or undocumented items explicitly and propose options (different approach, other library, ask user to add docs). **Ask about every unclear point** (behavior, hardware, connectivity, memory, power, timing, what counts as success). Repeat until nothing is ambiguous. Summarize the confirmed requirements and get approval.

## Phase 2: Plan and test spec
Create from templates in [resources/](./resources/):
- `PLAN.md` ([template](./resources/PLAN.template.md)): modular file structure, work packages, FQBN/options (taken from docs or `arduino-cli board listall` / `board details`), doc references.
- `TESTS.md` ([template](./resources/TESTS.template.md)): each requirement mapped to tests. Mark each `auto` (verifiable from the serial log) or `human` (visual/audio/physical). Make as many tests auto as possible.
- Firmware must be **designed for testing**: copy [test_config.h](./resources/test_config.h), [test_harness.h](./resources/test_harness.h), [tests.h](./resources/tests.h) and [tests.cpp](./resources/tests.cpp) (template) into the sketch; self-tests run once at boot under `TEST_MODE`, store their results in global variables, and the firmware dumps them over serial when the host sends `r`. Production code stays free of test code. `TEST_MODE` is defined in `test_config.h` (source, not a compiler flag), so every file must include `test_config.h` (directly or via `tests.h`/`test_harness.h`) before testing `#ifdef TEST_MODE`. The log format is fixed (see harness header); never invent other formats. Debug prints may exist but only `[TEST]` lines are judged.
Get user approval of PLAN.md and TESTS.md before implementing.

## Phase 3: Implementation (subagent)
Delegate implementation to a subagent (`invoke_subagent`, type `self`) with: PLAN.md, TESTS.md, the harness, the relevant doc files, and the rule "implement exactly the plan; use only APIs found in the docs (or verified in library headers); every test ID in TESTS.md must be reported through the harness; do not upload or touch hardware". If subagents are unavailable, implement inline. Review the result against the plan.

## Phase 4: Compile
`arduino-cli compile -b <FQBN>[:options] <sketch>`. `TEST_MODE` is enabled by `#define TEST_MODE 1` in the sketch's `test_config.h`, not by a compiler flag, so the build command is identical on every board. On errors: fix and recompile (compile fixes do not count as test loops, but stop and ask the user after 10 consecutive failed compiles).

## Phase 5: Upload (user-assisted when needed)
See [upload-and-ports.md](./references/upload-and-ports.md). Whether the board needs manual download/bootloader mode or a manual reset after upload comes from the board docs; **if the docs say it does (or upload fails to connect), ask the user to do it and wait for confirmation. Never upload without that confirmation.**
1. If required, ask the user to enter download mode.
2. `arduino-cli board list` to find the port, then `arduino-cli upload -b <FQBN> -p <PORT> <sketch>`.
3. If required, ask the user to reset the board so the firmware starts; wait for confirmation.
4. `$VPY scripts/capture_serial.py --list`: the port may have changed; use the current one.

## Phase 6: Capture and analyze
1. `$VPY scripts/capture_serial.py --port <PORT> --baud <BAUD> --out logs/run<N>.log --timeout 90 --send r` (`r` makes the firmware dump the results its boot-time tests stored in global variables, so the log is complete even if boot output was missed; baud must match the firmware's `Serial.begin`).
2. `$VPY scripts/parse_results.py logs/run<N>.log --expect <T ids from TESTS.md> --json logs/run<N>.json`. Do not parse logs by hand. Generic crash patterns are built in; if the board docs list platform-specific crash signatures, add them with `--crash-pattern <regex>` (repeatable). Exit codes: 0 all pass, 1 failures or crash, 2 incomplete (no DONE or missing ids).
3. Exit 2: check port/timeouts/boot loop; read the raw log to diagnose (crash output, wrong port or baud, firmware stuck).

## Phase 7: Fix loop (max 3 iterations)
If any auto test fails: analyze the log, fix the code (subagent or inline), compile, repeat Phase 5 to 6. **Stop after 3 loops** (and stop early if the same failure repeats unchanged), then report what failed, hypotheses and what you tried.

## Phase 8: Human tests
For each `H##` test, give the user one message in this exact form, then wait:

```
HUMAN TEST H01: <title>
Do:      <exact action>
Observe: <exactly what to look/listen for>
Reply:   H01: PASS  or  H01: FAIL - <what you saw/heard>   (photo/video/audio path welcome)
```
Batch all human tests in a single message in firmware order, triggering the firmware action via `--send r` (dumps stored results) or the input specified in the test. Photos can be read by the agent from a file path; sound must be described in words for now. Record replies in `logs/human_results.md`. A FAIL enters the fix loop like an auto failure (counts toward the 3 loops).

## Phase 9: Final report
Summarize: requirements covered, auto results (pass/fail per ID), human results, loops used, files changed, doc gaps found, open issues. Optionally rebuild for production (comment out `#define TEST_MODE 1` in `test_config.h`) and say so.

## Notes
- Always re-list ports after any mode change; never reuse an old port.
- Keep all logs in `<sketch>/logs/`.
- Missing or insufficient docs are a blocker for the affected feature: ask, do not guess APIs.
