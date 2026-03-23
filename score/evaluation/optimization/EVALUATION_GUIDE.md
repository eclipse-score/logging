# Optimization Evaluation Guide

## Scope
- **Source file:** `code_optimization.h`
- **Goal:** Verify tool identifies avoidable copies, redundant work, and complexity pitfalls.

## Test Conditions and Expected Review Comments
- **CO-01:** Large container by value; expected comment suggests `const&` where ownership not needed.
- **CO-02:** Repeated `.size()` in loop condition; expected comment suggests clearer/cheaper iteration style.
- **CO-03:** Temporary string in hot comparison; expected comment suggests literal or `string_view` path.
- **CO-04:** Double map lookup; expected comment suggests single-lookup pattern.
- **CO-05:** Redundant copy before return; expected comment mentions NRVO/move opportunities.
- **CO-06:** `std::list` where `std::vector` likely better cache behavior.
- **CO-07:** Virtual dispatch in tight loop; expected comment suggests template/static dispatch if appropriate.
- **CO-08:** Missing `reserve()` before push loop.
- **CO-09:** Redundant sort call.
- **CO-10:** Quadratic string concatenation in loop.

## Pass Criteria
- Tool should provide concrete complexity/performance rationale, not generic "optimize" comments.
