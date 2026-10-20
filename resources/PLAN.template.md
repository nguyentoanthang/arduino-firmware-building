# Implementation Plan: <project name>

## Requirements (confirmed with user)
- ...

## Docs consulted (<docs_folder>)
- `<file>`: what it provided (board facts, library API, ...)
- Gaps / unsupported items and how they were resolved: ...

## Hardware / board
- Board: <name>, FQBN: `<FQBN>` (+ options, e.g. memory/partition/USB settings)
- Upload procedure: <auto-reset | manual download mode (steps from docs)>
- Serial baud: <BAUD>
- Peripherals: ...
- Libraries (name, version): ...

## Architecture
| File | Responsibility |
|---|---|
| `<sketch>.ino` | setup/loop, wiring modules together |
| `<module>.h/.cpp` | ... |
| `test_config.h` | test mode configuration switch (from skill resources) |
| `test_harness.h` | test reporting (from skill resources) |
| `tests.h` / `tests.cpp` | test cases, compiled only with TEST_MODE |

## Work packages (for subagents)
1. ...
2. ...

## Risks / open questions
- ...
