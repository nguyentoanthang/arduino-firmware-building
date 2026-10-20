// tests.cpp - template for self-tests (arduino-firmware-building skill).
// Copy into the sketch folder together with test_harness.h and tests.h.
// Everything is compiled only when TEST_MODE is defined, so production builds stay clean.
//
// Rules:
//  - Every ID listed in TESTS.md must be reported exactly once per run.
//  - T## = automatic test (judged from the log), H## = human test (see TEST_HUMAN).
//  - Keep `detail` short and free of newlines.
//  - Replace the generic examples below with calls to the project's real modules/libraries
//    (APIs taken from references/docs/).
//  - Results are only stored (globals) while tests run; the firmware prints them when the host
//    sends 'r' (see test_harness.h). Call run_all_tests() once from setup().
//  - Keep `#include "tests.h"` first: it includes test_config.h, which defines TEST_MODE
//    before the #ifdef below.
#include "tests.h"
#ifdef TEST_MODE
#include <Arduino.h>
#include "test_harness.h"

void run_all_tests() {
  TEST_START("my_suite", __DATE__ " " __TIME__);

  // --- T01: example automatic test (value read from a module is in the expected range) ---
  int value = 42;  // replace with e.g. sensor.read()
  TEST_REPORT("T01", "value_in_range", value >= 0 && value <= 100, String("value=") + value);

  // --- T02: example timing check ---
  unsigned long t0 = millis();
  delay(50);
  unsigned long dt = millis() - t0;
  TEST_REPORT("T02", "delay_accuracy", dt >= 50 && dt < 70, String("dt=") + dt);

  // --- T03: example skipped test (feature unavailable) ---
  // TEST_SKIP("T03", "feature_x", "prerequisite not available");

  // --- H01: example human test: firmware does the action, human judges ---
  digitalWrite(LED_BUILTIN, HIGH);  // replace with the real visible/audible action
  TEST_HUMAN("H01", "LED should be on");
  delay(3000);  // leave the state visible long enough to be observed

  TEST_DONE();
}
#endif  // TEST_MODE
