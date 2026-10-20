#!/usr/bin/env python3
"""Create the skill's virtual environment (if missing) and install requirements.

Run with any system Python:  python setup_env.py
Idempotent. Prints the venv interpreter path on the last line (use it to run all scripts).
"""
import subprocess
import sys
import venv
from pathlib import Path

SKILL_DIR = Path(__file__).resolve().parent.parent
VENV_DIR = SKILL_DIR / ".venv"
REQ = Path(__file__).resolve().parent / "requirements.txt"
PY = VENV_DIR / ("Scripts/python.exe" if sys.platform == "win32" else "bin/python")


def main():
    if not PY.exists():
        print(f"Creating venv at {VENV_DIR}")
        venv.EnvBuilder(with_pip=True).create(VENV_DIR)
    subprocess.check_call([str(PY), "-m", "pip", "install", "--quiet", "-r", str(REQ)])
    subprocess.check_call([str(PY), "-c", "import serial; print('pyserial', serial.__version__)"])
    print(PY)


if __name__ == "__main__":
    main()
