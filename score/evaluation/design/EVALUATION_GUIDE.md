# Software Design Faults Evaluation Guide

## Scope
- **Source file:** `design_faults.h`
- **Goal:** Verify tool catches higher-level design smells and maintainability risks.

## Test Conditions and Expected Review Comments
- **DF-01:** Anemic domain model; expected comment suggests moving behavior to entity.
- **DF-02:** Temporal coupling (`Init()` before `Process()`); expected comment suggests enforced initialization strategy.
- **DF-03:** Feature envy; expected comment suggests moving behavior to owning type.
- **DF-04:** Duplicated policy constant (shotgun surgery risk).
- **DF-05:** Refused bequest / contract-breaking inheritance.
- **DF-06:** Inappropriate intimacy via `friend`.
- **DF-07:** Long parameter list; expected comment suggests parameter object.
- **DF-08:** Magic numbers in business logic.
- **DF-09:** Boolean-trap API shape.

## Pass Criteria
- Tool should provide architecture-level remediation ideas and identify LSP/contract implications where applicable.
