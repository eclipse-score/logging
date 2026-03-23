# Template Metaprogramming Evaluation Guide

## Scope
- **Source file:** `template_metaprogramming_issues.h`
- **Goal:** Verify tool catches maintainability, constraint, and modernization issues in template-heavy C++.

## Test Conditions and Expected Review Comments
- **TM-01:** Reinvented trait (`IsSameCustom`); expected comment suggests `std::is_same_v`.
- **TM-02:** Recursive `Factorial` TMP; expected comment suggests `constexpr` function for clarity and diagnostics.
- **TM-03:** Return-type SFINAE (`enable_if`) overloads; expected comment suggests `if constexpr`, constrained overloads, or helper traits for readability.
- **TM-04:** `AddOneGeneric` unconstrained; expected comment recommends constraining to arithmetic types.
- **TM-05:** `ValuePolicy<bool>` specialization changes semantic contract; expected comment should flag surprising specialization behavior.
- **TM-06:** Recursive variadic sum; expected comment suggests C++17 fold expression (`(... + args)`).
- **TM-07:** Hand-rolled typelist-length recursion; expected comment suggests standard facilities (`sizeof...`, tuple traits).
- **TM-08:** Pointer trait specialization misses qualifier nuance; expected comment suggests robust trait composition (`std::remove_cv_t`, references handling).
- **TM-09:** Boolean template parameter controls behavior; expected comment suggests policy types or explicit named traits for readability.
- **TM-10:** Missing `static_assert` preconditions in templates; expected comment suggests compile-time constraints for intended type categories.

## Pass Criteria
- Tool should report template API clarity/constraint issues, not only syntax-level findings.
- Tool should provide modernization guidance aligned with C++17 idioms.
