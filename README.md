# Arduino Firmware Building Skill

Autonomous build, flash, and test loop for any Arduino-compatible board using `arduino-cli`, serial test logs, and human-in-the-loop assistance.

---

## Overview

The **Arduino Firmware Building** skill enables AI coding assistants to autonomously engineer, implement, compile, upload, and verify firmware for embedded microcontrollers end-to-end.

The skill is completely **board-agnostic**: it makes no hardcoded assumptions about microcontroller architecture, pinouts, or peripheral libraries. All hardware specifics, API references, and board options are discovered interactively or derived strictly from user-provided documentation.

---

## Key Features

- **Strictly Document-Driven**: Works with any arduino compatible boards, given the board specifications and library documentation.
- **Interactive Board & FQBN Search**: Automatically discovers board names, Platform IDs, and exact FQBNs using `arduino-cli board search <name>`.
- **Comprehensive Plan for Human Review**: Generates detailed `PLAN.md` with full project directory trees, per-file API breakdowns, and Mermaid architecture diagrams before any code is written.
- **Human-in-the-Loop Review Gate**: Halts at the end of planning for human review and approval before touching code.
- **Firmware Built with Embedded Tests**: The firmware is built with self-tests embedded directly in the binary (`test_harness.h` & `tests.cpp`), executing automated checks (`T##`) at boot, storing structured records in static RAM, and dumping them on host request (`r`).
- **Comprehensive Edge-Case & Boundary Testing**: Rejects simplistic happy-path testing in favor of thorough test specifications covering boundary limits, negative/invalid inputs, fault injection/timeouts, state machine recovery, and hardware resilience.
- **Controlled via `TEST_MODE`**: The embedded test framework is controlled at the source level via `test_config.h` (`#define TEST_MODE 1`). For production builds, disabling `TEST_MODE` strips out all test code with zero flash or RAM overhead, without modifying board-specific compiler flags.
- **Universal Reset Protocol**: Always instructs the user to reset the board after flashing, guaranteeing firmware boots cleanly across both auto-reset and native USB bootloader boards.
- **Automated Serial Log Parsing**: `capture_serial.py` and `parse_results.py` stream test output and evaluate pass/fail criteria and crash detection automatically.
- **Autonomous Fix Loop**: Automatically diagnoses test failures from serial logs and applies code fixes (up to 3 iterations).

---

## Repository Structure

```
arduino-firmware-building/
├── SKILL.md                 # Master skill prompt and operational instructions
├── README.md                # Skill documentation and reference guide
├── .gitignore               # Excludes python virtual environment and caches
├── references/
│   └── upload-and-ports.md  # Guidelines on native USB port switching and bootloaders
├── resources/
│   ├── PLAN.template.md     # Comprehensive architecture plan template for human review
│   ├── TESTS.template.md    # Test specification template
│   ├── test_config.h        # Source-level TEST_MODE toggle and RAM config
│   ├── test_harness.h       # C++11 board-agnostic test harness
│   ├── tests.h              # Test suite declarations
│   └── tests.cpp            # Self-test implementation template
└── scripts/
    ├── setup_env.py         # Sets up local Python virtual environment (.venv)
    ├── capture_serial.py    # Serial logger with host trigger support
    ├── parse_results.py     # Log parser with generic and custom crash filters
    └── requirements.txt     # Python dependencies (pyserial)
```

---

## Prerequisites & Board Setup

- **`arduino-cli`**: Must be installed and available in your system `PATH` (along with Python 3 for virtualenv serial capture scripts).
- **Board Manager & Hardware Setup**:
  1. **If using a custom Board Manager URL:** Ensure the URL is added to the `arduino-cli` configuration before running:
     ```bash
     arduino-cli config add board_manager.additional_urls <board_manager_url>
     arduino-cli core update-index
     ```
     The skill's Phase 0 search will discover the FQBN and install the core automatically.
  2. **If using a custom PCB with a generic chip:** Specify the base chip's FQBN (e.g. `esp32:esp32:esp32s3`, `rp2040:rp2040:generic`) and install its core (e.g. `arduino-cli core install esp32:esp32`). Provide the pin mapping in `docs/board.md`. The skill will generate `config.h` matching the custom wiring.

---

## How to Use This Skill

### 1. Prepare Your Project
In your project directory, create a `docs/` folder containing your board pinout and library API references:
```
my_project/
└── docs/
    ├── board.md          # Pin definitions, buttons, display, FQBN notes, bootloader procedure
    └── library_api.md    # API methods and initialization examples
```

---

### 2. Setup & Invocation

Clone or place this repository in your project or your AI tool's global skills folder:

```bash
git clone https://github.com/nguyentoanthang/arduino-firmware-building.git <skills_folder>/arduino-firmware-building
```

| Tool | Project-Level Folder | Global (User-Level) Folder |
| :--- | :--- | :--- |
| **Claude Code** | `.claude/skills/arduino-firmware-building` | `~/.claude/skills/arduino-firmware-building` |
| **OpenAI Codex** | `.codex/skills/arduino-firmware-building` | `~/.codex/skills/arduino-firmware-building` |
| **Google Antigravity** | `.gemini/skills/arduino-firmware-building` | `~/.gemini/config/skills/arduino-firmware-building` |

**Example Usage:**
```
/arduino-firmware-building build the firmware for <board name>, the requirement are <user requirement>
```

---

## License

MIT License. Open for development, extension, and deployment across embedded Arduino projects.
