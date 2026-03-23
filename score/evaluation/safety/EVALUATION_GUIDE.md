# Safe C++ Evaluation Guide

## Scope
- **Source file:** `safe_cpp.h`
- **Goal:** Verify tool catches UB-prone code and defensive-programming gaps.

## Test Conditions and Expected Review Comments
- **SC-02:** Potential signed overflow UB.
- **SC-03:** Implicit narrowing conversion.
- **SC-04:** Unchecked index access via `operator[]`.
- **SC-05:** Potential throw from destructor path.
- **SC-06:** Unhandled exception in thread function.
- **SC-07:** Strict-aliasing violation through `reinterpret_cast`.
- **SC-09:** Throwing parser API used without handling strategy.
- **SC-10:** Implicit bool-to-int arithmetic readability/safety concern.

## Pass Criteria
- Tool should prioritize true safety/UB defects above minor style findings and suggest safer alternatives.
