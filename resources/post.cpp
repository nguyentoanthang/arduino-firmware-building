// post.cpp - Power-On Self-Test (POST) implementation template.
// Permanent production code: executes once in setup() on every boot/reset.
// Replace the template checks below with project-specific hardware probes
// derived from user-provided board and library documentation.
#include "post.h"

static PostResult s_post_result = { true, POST_PASS, "POST: OK" };

PostResult post_run() {
  uint8_t flags = POST_PASS;

  // -------------------------------------------------------------------------
  // 1. Bus Integrity & Device ACK Check (e.g. I2C Wire)
  // -------------------------------------------------------------------------
  // Example: probe documented I2C address with non-blocking timeout
  // Wire.beginTransmission(DEVICE_I2C_ADDR);
  // if (Wire.endTransmission() != 0) {
  //   flags |= POST_FAIL_BUS;
  // }

  // -------------------------------------------------------------------------
  // 2. Sensor / Peripheral ID Verification (e.g. WHO_AM_I register)
  // -------------------------------------------------------------------------
  // Example: read documented chip ID register
  // if (sensor_get_device_id() != EXPECTED_CHIP_ID) {
  //   flags |= POST_FAIL_SENSOR;
  // }

  // -------------------------------------------------------------------------
  // 3. Storage / Memory Sanity Check
  // -------------------------------------------------------------------------
  // Example: verify filesystem or configuration integrity
  // if (!storage_verify_magic()) {
  //   flags |= POST_FAIL_STORAGE;
  // }

  s_post_result.all_passed = (flags == POST_PASS);
  s_post_result.failure_flags = flags;
  if (!s_post_result.all_passed) {
    snprintf(s_post_result.message, sizeof(s_post_result.message), "POST FAIL flags=0x%02X", flags);
    Serial.println(s_post_result.message);
  } else {
    strncpy(s_post_result.message, "POST: All HW OK", sizeof(s_post_result.message));
    Serial.println(s_post_result.message);
  }

  return s_post_result;
}

const PostResult& post_get_result() {
  return s_post_result;
}

void post_handle_failure(const PostResult& result) {
  // Permanent production behavior when physical hardware fails at boot:
  // Log error diagnostic and trigger visible warning indicator (e.g. LED blink code)
  Serial.print(F("[HARDWARE FAULT] "));
  Serial.println(result.message);

  #ifdef LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  // Optional warning loop if critical hardware is missing
  // while (1) {
  //   digitalWrite(LED_BUILTIN, HIGH);
  //   delay(100);
  //   digitalWrite(LED_BUILTIN, LOW);
  //   delay(100);
  // }
  #endif
}
