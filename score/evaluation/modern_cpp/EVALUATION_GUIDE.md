# Modern C++ Evaluation Guide

## Scope
- **Source file:** `modern_cpp_syntax.h`
- **Goal:** Verify tool identifies missed C++11/14/17 language/library opportunities.

## Test Conditions and Expected Review Comments
- **MC-01:** Raw iterator loop; expected comment suggests range-for.
- **MC-02:** Legacy pointer idiom awareness; expected comment should discourage removed/obsolete ownership styles.
- **MC-03:** Verbose functor; expected comment suggests lambda.
- **MC-04:** Manual tagged union; expected comment suggests `std::variant` + `std::visit`.
- **MC-05:** Move operations not `noexcept`; expected comment explains container-move optimization impact.
- **MC-06:** `typedef` legacy style; expected comment suggests `using` alias.
- **MC-07:** Pair-based sentinel return; expected comment suggests `std::optional` / clearer API.
- **MC-08:** `map::operator[]` unintended insertion; expected comment suggests `find()`/`at()` depending on intent.
- **MC-09:** `std::bind` for simple closure; expected comment suggests lambda.
- **MC-10:** Missing `[[nodiscard]]` on status-returning API.

## Pass Criteria
- Tool should connect suggestions to C++17 semantics and performance/safety implications.
