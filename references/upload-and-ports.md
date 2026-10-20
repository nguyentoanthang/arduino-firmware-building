# Upload, download mode and port changes

## Why this matters
Some boards (typically ones with native USB) appear as a *different USB device* in each mode, and the OS may assign a *different serial port* to each. Whether this applies, and how to enter download/bootloader mode, comes from the user-provided board documentation, not from this file.

| Mode | How it gets there | Example port |
|---|---|---|
| Normal (running firmware) | after reset / power on | `COM7` |
| Download / bootloader | board-specific button sequence (see board docs) | `COM5` |

If the board does not auto-reset after upload, it stays in bootloader until the user resets it, so the port after upload may differ from the port used to upload.

## Example session (board that needs manual download mode and manual reset)
1. Agent: "Put the board in download mode (<procedure from docs>) and tell me when done."
2. User: "done"
3. Agent runs `arduino-cli board list` -> sees `COM5`.
4. Agent runs `arduino-cli upload -b <FQBN> -p COM5 <sketch>` -> succeeds, but the board may stay in the bootloader.
5. Agent: "Upload finished. Reset the board (<procedure from docs>) and tell me when done."
6. User: "done"
7. Agent runs `$VPY capture_serial.py --list` ($VPY = the skill's venv python) -> sees `COM7`. Agent captures from `COM7`.
8. If the capture reports `port lost` or cannot open: list ports again and retry with the new port.

Boards that auto-reset (e.g. classic Uno) skip steps 1, 2, 5 and 6. Never assume the old port number; always re-list after every mode change.
