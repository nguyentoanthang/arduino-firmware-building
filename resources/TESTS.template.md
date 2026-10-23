# Test Specification: <project name>

Test IDs: `T##` = automatic (verified via serial log), `H##` = human (visual/audio/tactile verification).

---

## 1. Testing Policy & Coverage Requirements

> [!IMPORTANT]
> **Basic tests are insufficient.** Basic happy-path tests easily pass while masking critical firmware bugs. Every test specification must systematically include edge cases, boundary conditions, negative inputs, and fault resilience checks.

### Mandatory Test Categories
Every firmware requirement must be tested against applicable categories below:
1. **Nominal / Functional (`NOM`):** Verifies standard expected operation within normal parameters.
2. **Boundary & Extremes (`BND`):** Tests absolute minimum, maximum, zero values, integer overflow/underflow, array/buffer saturation limits, empty strings, and maximum allowed lengths.
3. **Negative & Invalid Input (`NEG`):** Verifies rejection of invalid parameters, `NULL` pointers, unsupported command codes, and malformed inputs with clean error codes (never panicking or corrupting memory).
4. **Fault Simulation & Hardware Resilience (`FLT`):** Tests communication timeouts, I2C NACK / missing peripheral response handling, and non-blocking recovery (firmware must NEVER enter unbounded `while` loops waiting for hardware).
5. **State Machine & Lifecycle (`STA`):** Tests double initialization (calling `begin()` twice), uninitialized operations (calling `read()` before `begin()`), out-of-order execution, and recovery from error states.
6. **Timing & Asynchrony (`TIM`):** Tests rapid successive triggers, switch debounce boundaries, `millis()` wraparound safety, and non-blocking loop execution duration.
7. **Human Edge-Case Verification (`HUM`):** Human inspection under extreme conditions (e.g., maximum string length rendering without display corruption, error LED behavior during simulated faults).

---

## 2. Test Specification Table

| ID | Req | Category | Test Name & Objective | Stimulus / Test Procedure / Edge Condition | Pass Criteria | Type |
|---|---|---|---|---|---|---|
| `T01` | R1 | NOM | Sensor Nominal Reading | Read sensor under normal operating conditions | Value within documented nominal range; returns success code | auto |
| `T02` | R1 | BND | Min Boundary Clamping | Inject input below lower threshold (e.g. 0 or negative) | Clamped to safe lower limit; no arithmetic underflow | auto |
| `T03` | R1 | BND | Max Boundary Clamping | Inject input exceeding upper limit (e.g. > max sensor range) | Clamped to safe upper limit; no arithmetic overflow | auto |
| `T04` | R2 | BND | Buffer Limit & Null Termination | Pass string/data equal to or exceeding internal buffer size | Truncated cleanly to `BUF_SIZE - 1` with null-terminator; zero buffer overrun | auto |
| `T05` | R2 | NEG | Null Pointer Safety Guard | Pass `NULL` pointer to public API functions | Function returns safe error code; zero crash or hard fault | auto |
| `T06` | R2 | NEG | Invalid Command Rejection | Send undefined opcode or out-of-range configuration enum | Function rejects input gracefully; state remains valid | auto |
| `T07` | R3 | FLT | Peripheral Timeout / Bus NACK | Simulate unresponsive slave or disconnected sensor | Non-blocking timeout expires in <= 100ms; returns error flag without hang | auto |
| `T08` | R4 | STA | Idempotent Re-initialization | Call module `begin()` / `init()` twice consecutively | System re-initializes cleanly; no memory leak or double-free crash | auto |
| `T09` | R4 | STA | Uninitialized Operation Safety | Call `process()` or `read()` before calling `begin()` | Safe rejection with error code; system does not lock up | auto |
| `T10` | R5 | TIM | Debounce Rapid Glitch Rejection | Simulate rapid input toggles within < debounce window (e.g. 10ms) | Noise transitions ignored; counter/state changes exactly once | auto |
| `H01` | R6 | HUM | Normal UI / LED Indicator | Observe display layout and status LED in nominal state | Display shows expected labels; status LED solid ON | human |
| `H02` | R6 | HUM | Edge Case: Maximum String UI Rendering | Feed string of maximum supported length to UI display module | Text wraps or truncates cleanly without screen clipping or memory corruption | human |
| `H03` | R7 | HUM | Edge Case: Fault Indication on Failure | Trigger simulated hardware error / timeout | Fault LED blinks rapid warning pattern or display shows error banner | human |

---

## 3. Edge Cases & Failure Modes Matrix

| Potential Failure / Edge Condition | Risk | Firmware Handling & Mitigation | Verified By |
|---|---|---|---|
| **Peripheral Disconnect / Bus Lockup** | System hangs indefinitely in `while(!wire.available())` | Non-blocking `millis()` timeout on all bus operations; safe error fallback | `T07` |
| **Buffer Overflow / String Overrun** | Memory corruption, stack smashing, crash | Strict bounds checking (`strncpy`, buffer size guards, explicit null termination) | `T04` |
| **Invalid Inputs & Null Pointers** | Null-pointer dereference, kernel panic | Defensive parameter validation at API entry points | `T05`, `T06` |
| **Rapid Input Chatter / Noise** | False multi-triggers, state oscillation | Time-based debounce state machine | `T10` |
| **Out-of-Order Lifecycle Calls** | Crash or undefined behavior | State flags (`is_initialized`) guarding execution paths | `T08`, `T09` |
| **Display Overflow on Long Strings** | Visual clipping or heap corruption | Explicit string length clamping and safe text rendering bounds | `H02` |

---

## 4. Expected Automated Test IDs

Used for automated log parsing with `parse_results.py`:
```bash
--expect T01,T02,T03,T04,T05,T06,T07,T08,T09,T10
```
> [!NOTE]
> Adjust `TEST_MAX_RECORDS` in `test_config.h` if the total number of tests (`T##` + `H##`) exceeds the default capacity.
