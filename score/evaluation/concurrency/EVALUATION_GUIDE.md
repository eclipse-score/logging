# Concurrency Evaluation Guide

## Scope
- **Source file:** `concurrency_issues.h`
- **Goal:** Verify tool catches race conditions, synchronization mistakes, and thread-lifetime bugs.

## Test Conditions and Expected Review Comments
- **CI-01:** Data race on plain shared counter.
- **CI-02:** Manual lock/unlock without RAII (exception safety deadlock risk).
- **CI-03:** Broken double-checked init pattern / memory-ordering concerns.
- **CI-04:** Deadlock from inconsistent lock ordering.
- **CI-05:** Condition-variable wait not predicate-safe for spurious wakeups.
- **CI-06:** Detached thread captures stack reference (dangling reference risk).
- **CI-07:** Check-then-act race despite atomic variable.

## Pass Criteria
- Tool should identify concrete interleavings/failure modes and propose robust synchronization patterns (`lock_guard`, `scoped_lock`, predicate waits, CAS loops).
