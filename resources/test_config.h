// test_config.h - single switch for test mode, board-independent.
// Copy into the sketch folder. Every file that uses the test harness includes this header
// first, so TEST_MODE is defined consistently in all translation units (each .cpp is compiled
// separately, so a define in the .ino alone would not reach the other files).
// No compiler flag is needed, which keeps the workflow identical on every board.
// Production build: comment out the next line (or change it to 0).
#pragma once
#define TEST_MODE 1

// Optional: size of the in-RAM result store (see test_harness.h).
// Ensure TEST_MAX_RECORDS is at least equal to the total automated (T##) and human (H##) tests.
// - Default in test_harness.h: 24 records.
// - Modern MCUs (ESP32, RP2040, STM32, SAMD): can easily use 32, 48, or more.
// - RAM-constrained MCUs (ATmega328P / 2KB RAM): keep around 12-16 and TEST_TEXT_LEN at 32.
// #define TEST_MAX_RECORDS 24   // max number of T##/H## entries
// #define TEST_TEXT_LEN 48      // max chars kept for each detail/prompt text
