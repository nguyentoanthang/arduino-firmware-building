# Test Specification: <project name>

Test IDs: `T##` = automatic (verified via serial log), `H##` = human (see HUMAN protocol).
Every requirement must map to at least one test.

| ID | Requirement | Type | Procedure | Pass criteria (as seen in log or by human) |
|---|---|---|---|---|
| T01 | R1 | auto | firmware does X in boot self-test | `RESULT id=T01 status=PASS` |
| H01 | R2 | human | firmware shows X on screen | human answers PASS |

Expected auto IDs (for `parse_results.py --expect`): T01,T02,...
