# Design Patterns Evaluation Guide

## Scope
- **Source file:** `design_patterns.h`
- **Goal:** Verify tool catches pattern misuse and unsafe pattern implementations.

## Test Conditions and Expected Review Comments
- **DP-01:** Broken singleton initialization/lifetime; expected comment suggests Meyers singleton or avoiding singleton.
- **DP-02:** Observer uses raw listener pointers; expected comment flags lifetime/UAF risk and suggests safe registration strategy.
- **DP-03:** Factory returns owning raw pointer; expected comment suggests smart-pointer return type.
- **DP-04:** Virtual call from constructor; expected comment explains dispatch rules during construction.
- **DP-05:** Strategy stored as raw pointer; expected comment requests explicit ownership/lifetime semantics.
- **DP-06:** Fake CRTP/downcast misuse; expected comment requests proper CRTP or safe polymorphism.

## Pass Criteria
- Tool should identify architectural-level issues, not only local code smells.
