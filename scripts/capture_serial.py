#!/usr/bin/env python3
"""Capture serial output from an Arduino-compatible board to a log file.

Requires: pip install pyserial

Examples:
  python capture_serial.py --list
  python capture_serial.py --port COM5 --out run1.log --timeout 90
  python capture_serial.py --port COM5 --out run1.log --send r
"""
import argparse
import sys
import time

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("ERROR: pyserial missing. Run: pip install pyserial", file=sys.stderr)
    sys.exit(3)

DONE_MARKER = "[TEST] DONE"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--list", action="store_true", help="list serial ports and exit")
    ap.add_argument("--port", help="e.g. COM5")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--out", default="serial.log", help="log file path")
    ap.add_argument("--timeout", type=float, default=60, help="max capture seconds")
    ap.add_argument("--until", default=DONE_MARKER,
                    help="stop when a line contains this text ('' = run until timeout)")
    ap.add_argument("--wait-port", type=float, default=15,
                    help="seconds to wait for the port to appear/open")
    ap.add_argument("--send", default="",
                    help="text sent right after opening (e.g. 'r' triggers a test rerun)")
    args = ap.parse_args()

    if args.list:
        ports = list(list_ports.comports())
        if not ports:
            print("No serial ports found.")
        for p in ports:
            print(f"{p.device}\t{p.description}\tVID:PID={p.vid}:{p.pid}")
        return 0

    if not args.port:
        print("ERROR: --port required (use --list)", file=sys.stderr)
        return 3

    # Open without toggling DTR/RTS to avoid resetting the board into bootloader.
    ser = serial.Serial()
    ser.port, ser.baudrate, ser.timeout = args.port, args.baud, 0.2
    ser.dtr = False
    ser.rts = False

    deadline = time.time() + args.wait_port
    while True:
        try:
            ser.open()
            break
        except (serial.SerialException, OSError) as e:
            if time.time() > deadline:
                print(f"ERROR: cannot open {args.port}: {e}", file=sys.stderr)
                return 3
            time.sleep(0.5)

    if args.send:
        time.sleep(0.3)
        ser.write(args.send.encode())

    end = time.time() + args.timeout
    found = False
    buf = b""
    try:
        with open(args.out, "w", encoding="utf-8", errors="replace") as f:
            while time.time() < end:
                try:
                    chunk = ser.read(256)
                except (serial.SerialException, OSError) as e:
                    print(f"ERROR: port lost ({e}); device may have reset/changed port",
                          file=sys.stderr)
                    return 4
                if not chunk:
                    continue
                buf += chunk
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    text = line.decode("utf-8", errors="replace").rstrip("\r")
                    f.write(text + "\n")
                    f.flush()
                    if args.until and args.until in text:
                        found = True
                if found:
                    break
    finally:
        ser.close()

    print(f"Captured to {args.out}; end marker {'found' if found else 'NOT found (timeout)'}")
    return 0 if (found or not args.until) else 2


if __name__ == "__main__":
    sys.exit(main())
