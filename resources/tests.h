#pragma once
#include "test_config.h"
// Declares the self-test entry point; empty in production builds.
#ifdef TEST_MODE
void run_all_tests();
#endif
