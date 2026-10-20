#!/usr/bin/env python3
"""Parse a captured serial log for [TEST] lines and summarize results.

Log line format (emitted by resources/test_harness.h):
  [TEST] START suite=<name> build=<id>
  [TEST] RESULT id=T01 status=PASS|FAIL|SKIP name=<name> detail=<text>
  [TEST] HUMAN id=H01 prompt=<text>
  [TEST] DONE total=N pass=P fail=F skip=S

Usage:
  python parse_results.py run1.log [--expect T01,T02,T03] [--json out.json]
                          [--crash-pattern REGEX ...]

Crash detection: non-[TEST] lines are checked against generic crash patterns (panic, fatal
error, hard fault, exception, assertion failed, stack overflow, watchdog). Add platform-specific
crash signatures from the board docs with --crash-pattern (repeatable, regular expression).

Exit codes: 0 all pass, 1 any FAIL, 2 incomplete (no DONE or missing expected ids).
"""
import argparse
import json
import re
import sys

KV = re.compile(r"(\w+)=(.*?)(?=\s+\w+=|$)")

DEFAULT_CRASH_PATTERNS = [
    r"\bpanic\b",
    r"fatal error",
    r"hard ?fault",
    r"\bexception\b",
    r"assert(ion)? failed",
    r"stack (smash|overflow)",
    r"watchdog",
]


def parse_kv(s):
    return {m.group(1): m.group(2).strip() for m in KV.finditer(s)}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("log")
    ap.add_argument("--expect", default="", help="comma-separated test ids that must appear")
    ap.add_argument("--json", help="write summary JSON here")
    ap.add_argument("--crash-pattern", action="append", default=[], metavar="REGEX",
                    help="extra regex marking a crash line (repeatable); "
                         "use platform signatures from the board docs")
    a = ap.parse_args()
    crash_re = re.compile("|".join(f"(?:{p})" for p in DEFAULT_CRASH_PATTERNS + a.crash_pattern),
                          re.IGNORECASE)

    results, humans, start, done = {}, {}, None, None
    other_errors = []
    with open(a.log, encoding="utf-8", errors="replace") as f:
        for raw in f:
            line = raw.strip()
            if "[TEST]" not in line:
                if crash_re.search(line):
                    other_errors.append(line)
                continue
            body = line.split("[TEST]", 1)[1].strip()
            kind, _, rest = body.partition(" ")
            kv = parse_kv(rest)
            if kind == "START":
                start = kv
                results.clear()  # keep only the latest run in the log
            elif kind == "RESULT":
                results[kv.get("id", "?")] = kv
            elif kind == "HUMAN":
                humans[kv.get("id", "?")] = kv
            elif kind == "DONE":
                done = kv

    expected = [x for x in a.expect.split(",") if x]
    missing = [x for x in expected if x not in results]
    fails = [r for r in results.values() if r.get("status") == "FAIL"]
    passed = [r for r in results.values() if r.get("status") == "PASS"]
    skipped = [r for r in results.values() if r.get("status") == "SKIP"]

    summary = {
        "started": start is not None,
        "done": done is not None,
        "pass": len(passed), "fail": len(fails), "skip": len(skipped),
        "missing": missing,
        "failures": fails,
        "human_pending": list(humans.values()),
        "crash_lines": other_errors,
    }
    if a.json:
        with open(a.json, "w", encoding="utf-8") as f:
            json.dump(summary, f, indent=2)

    print(f"started={summary['started']} done={summary['done']} "
          f"pass={len(passed)} fail={len(fails)} skip={len(skipped)}")
    for r in fails:
        print(f"FAIL {r.get('id')} {r.get('name','')}: {r.get('detail','')}")
    for m in missing:
        print(f"MISSING {m}")
    for h in humans.values():
        print(f"HUMAN {h.get('id')}: {h.get('prompt','')}")
    for c in other_errors[:5]:
        print(f"CRASH? {c}")

    if fails or other_errors:
        return 1
    if not summary["done"] or missing or not summary["started"]:
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
