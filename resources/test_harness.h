// test_harness.h - serial test reporting for the arduino-firmware-building skill.
// Board-agnostic: uses only Serial.print/println (no printf), so it works on AVR, ARM, ESP, etc.
// Plain C++11 (no inline variables), so it also builds with older cores such as AVR.
// Enable by defining TEST_MODE in test_config.h (included below, works on every board; no
// compiler flag needed).
// Call Serial.begin(<BAUD>) in setup() before results are dumped.
//
// Design: tests run at start-up and store their results in global variables (records).
// Nothing is printed while tests run. When the host sends 'r', the firmware dumps the stored
// results over serial in the fixed log format below. TEST_SERVICE() does this from loop().
// RAM use is about TEST_MAX_RECORDS * (TEST_TEXT_LEN + 34) bytes; on small-RAM boards lower
// TEST_MAX_RECORDS / TEST_TEXT_LEN in test_config.h (text longer than TEST_TEXT_LEN is cut).
//
// Log format (parsed by scripts/parse_results.py):
//   [TEST] START suite=<name> build=<id>
//   [TEST] RESULT id=T01 status=PASS|FAIL|SKIP name=<name> detail=<text>
//   [TEST] HUMAN id=H01 prompt=<text>
//   [TEST] DONE total=N pass=P fail=F skip=S
//   [TEST] IDLE            (printed every 2 s while waiting; send 'r' to dump results)
#pragma once
#include <Arduino.h>
#include "test_config.h"

#ifdef TEST_MODE

#ifndef TEST_MAX_RECORDS
#define TEST_MAX_RECORDS 12
#endif
#ifndef TEST_TEXT_LEN
#define TEST_TEXT_LEN 48
#endif

enum TestKind : uint8_t { TK_PASS, TK_FAIL, TK_SKIP, TK_HUMAN };

struct TestRecord {
  TestKind kind;
  char id[8];
  char name[24];
  char text[TEST_TEXT_LEN];  // detail (RESULT) or prompt (HUMAN)
};

// Global result store. Accessed through an inline function with a function-local static so
// that every translation unit shares exactly one instance (valid C++11, no C++17 needed).
struct TestStore {
  TestRecord records[TEST_MAX_RECORDS];
  int count;
  bool done;
  char suite[24];
  char build[24];
};

inline TestStore& _t_store() {
  static TestStore s = {};
  return s;
}

inline void _t_copy(char* dst, size_t n, const char* src) {
  strncpy(dst, src ? src : "", n - 1);
  dst[n - 1] = '\0';
}

inline void _t_add(TestKind kind, const char* id, const char* name, const char* text) {
  TestStore& s = _t_store();
  if (s.count >= TEST_MAX_RECORDS) return;
  TestRecord& r = s.records[s.count++];
  r.kind = kind;
  _t_copy(r.id, sizeof(r.id), id);
  _t_copy(r.name, sizeof(r.name), name);
  _t_copy(r.text, sizeof(r.text), text);
}

inline void TEST_START(const char* suite, const char* build) {
  TestStore& s = _t_store();
  s.count = 0;
  s.done = false;
  _t_copy(s.suite, sizeof(s.suite), suite);
  _t_copy(s.build, sizeof(s.build), build);
}

inline void TEST_REPORT(const char* id, const char* name, bool ok, const String& detail = "") {
  _t_add(ok ? TK_PASS : TK_FAIL, id, name, detail.c_str());
}

inline void TEST_SKIP(const char* id, const char* name, const char* why) {
  _t_add(TK_SKIP, id, name, why);
}

// Register a test that a human must judge. The firmware performs the action; the human reports.
inline void TEST_HUMAN(const char* id, const char* prompt) {
  _t_add(TK_HUMAN, id, "", prompt);
}

inline void TEST_DONE() { _t_store().done = true; }

// Print everything stored so far in the fixed log format.
inline void TEST_DUMP() {
  TestStore& s = _t_store();
  int pass = 0, fail = 0, skip = 0;
  Serial.print("[TEST] START suite=");
  Serial.print(s.suite);
  Serial.print(" build=");
  Serial.println(s.build);
  for (int i = 0; i < s.count; i++) {
    const TestRecord& r = s.records[i];
    if (r.kind == TK_HUMAN) {
      Serial.print("[TEST] HUMAN id=");
      Serial.print(r.id);
      Serial.print(" prompt=");
      Serial.println(r.text);
      continue;
    }
    const char* st = r.kind == TK_PASS ? "PASS" : (r.kind == TK_FAIL ? "FAIL" : "SKIP");
    if (r.kind == TK_PASS) pass++;
    else if (r.kind == TK_FAIL) fail++;
    else skip++;
    Serial.print("[TEST] RESULT id=");
    Serial.print(r.id);
    Serial.print(" status=");
    Serial.print(st);
    Serial.print(" name=");
    Serial.print(r.name);
    Serial.print(" detail=");
    Serial.println(r.text);
  }
  if (s.done) {
    Serial.print("[TEST] DONE total=");
    Serial.print(pass + fail + skip);
    Serial.print(" pass=");
    Serial.print(pass);
    Serial.print(" fail=");
    Serial.print(fail);
    Serial.print(" skip=");
    Serial.println(skip);
  }
}

// Call from loop(): dumps stored results when host sent 'r'; prints IDLE heartbeat otherwise.
inline void TEST_SERVICE() {
  static unsigned long last = 0;
  while (Serial.available()) {
    if (Serial.read() == 'r') {
      TEST_DUMP();
      return;
    }
  }
  if (millis() - last > 2000) {
    last = millis();
    Serial.println("[TEST] IDLE");
  }
}

#define TEST_ASSERT(id, name, cond, detail) TEST_REPORT(id, name, (cond), (detail))

#else  // production build: everything compiles away
inline void TEST_START(const char*, const char*) {}
inline void TEST_REPORT(const char*, const char*, bool, const String& = "") {}
inline void TEST_SKIP(const char*, const char*, const char*) {}
inline void TEST_HUMAN(const char*, const char*) {}
inline void TEST_DONE() {}
inline void TEST_DUMP() {}
inline void TEST_SERVICE() {}
#define TEST_ASSERT(id, name, cond, detail) ((void)0)
#endif
