# SOLID Evaluation Guide

## Scope
- **Source file:** `solid_violations.h`
- **Goal:** Verify tool catches SOLID design-principle violations.

## Test Conditions and Expected Review Comments
- **SV-01 (SRP):** `LogManager` is a God class; expected comment recommends splitting config/format/routing/I-O/stats concerns.
- **SV-02 (OCP):** `AreaCalculator` uses type-tag branching; expected comment recommends polymorphic `IShape::Area()`.
- **SV-03 (LSP):** `Derived::Compute()` changes base contract and throws unexpectedly; expected comment flags substitutability break.
- **SV-04 (ISP):** `IComponent` is fat; expected comment suggests small interfaces (`ILifecycle`, `ISerializable`, etc.).
- **SV-05 (DIP):** `HighLevelProcessor` constructs concrete `FileSink`; expected comment recommends abstraction + dependency injection.

## Pass Criteria
- Tool should identify each tag (`SV-01`..`SV-05`) and provide a concrete refactoring suggestion, not only style warnings.
