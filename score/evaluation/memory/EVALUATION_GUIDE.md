# Memory Evaluation Guide

## Scope
- **Source file:** `memory_issues.h`
- **Goal:** Verify tool catches lifetime/ownership and UB-prone memory patterns.

## Test Conditions and Expected Review Comments
- **MI-01:** Owning raw pointer in `RawOwner`; expected comment suggests RAII (`std::vector`/`std::unique_ptr`).
- **MI-02:** Missing virtual destructor in polymorphic base; expected comment flags UB on delete-through-base.
- **MI-03:** Rule-of-Five break causes double-free risk; expected comment asks for Rule-of-Zero/Rule-of-Five fix.
- **MI-04:** Constructor exception leak path; expected comment recommends RAII member ownership.
- **MI-05:** Vector pointer invalidation then dereference; expected comment flags dangling pointer/UAF.
- **MI-08:** Copy-assignment self-assignment bug; expected comment suggests guard or copy-swap idiom.

## Pass Criteria
- Tool should report high-severity memory/lifetime defects and explain runtime impact (UB, leaks, double-free).
