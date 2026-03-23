# Non-Idiomatic C++ Evaluation Guide

## Scope
- **Source file:** `non_idiomatic_cpp.h`
- **Goal:** Verify tool catches non-idiomatic legacy patterns and recommends modern C++ style.

## Test Conditions and Expected Review Comments
- **NI-01:** C-style casts; expected comment suggests named casts.
- **NI-02:** `NULL` usage; expected comment suggests `nullptr`.
- **NI-03:** `#define` constants; expected comment suggests `constexpr`.
- **NI-04:** C-style arrays; expected comment suggests `std::array`.
- **NI-05:** `sprintf`; expected comment flags safety and suggests safer formatting.
- **NI-06:** Manual loops for accumulation; expected comment suggests STL algorithms.
- **NI-07:** Mutable global state in header; expected comment flags coupling/ODR/thread-safety concerns.
- **NI-08:** `std::string` pass-by-value; expected comment suggests `const&` or `std::string_view`.
- **NI-09:** `std::endl` flush in loop; expected comment suggests `"\n"`.
- **NI-10:** Out-parameter parse API; expected comment suggests value-return (`std::optional`).

## Pass Criteria
- Tool should emit actionable modernization guidance beyond formatting-only remarks.
