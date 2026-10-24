// post.h - Power-On Self-Test (POST) for embedded Arduino firmware.
// Board-agnostic hardware health checks executed once at power-on / boot in setup().
// Unlike test_harness.h (which is stripped in production when TEST_MODE=0),
// POST is permanent production firmware logic verifying physical hardware integrity.
#pragma once
#include <Arduino.h>

enum PostFailureFlags : uint8_t {
  POST_PASS = 0,
  POST_FAIL_BUS = (1 << 0),         // I2C/SPI communication bus failure
  POST_FAIL_SENSOR = (1 << 1),      // Sensor missing, disconnected, or WHO_AM_I mismatch
  POST_FAIL_STORAGE = (1 << 2),     // Flash, EEPROM, NVS, or SD mount failure
  POST_FAIL_PERIPHERAL = (1 << 3)   // Display, actuator, or critical GPIO pin failure
};

struct PostResult {
  bool all_passed;
  uint8_t failure_flags;            // Bitmask of PostFailureFlags
  char message[48];                 // Diagnostic status string
};

// Executes all physical hardware health checks once in setup().
// Safe across all boards: non-blocking bus checks, device ID validation, and rail sanity.
PostResult post_run();

// Query the result of the last POST run.
const PostResult& post_get_result();

// Production fault handler: invoked if post_run() fails.
// Enters a safe degraded mode, signals warning LED blink pattern, or displays error message.
void post_handle_failure(const PostResult& result);
