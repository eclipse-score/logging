# Exception & Error Handling Evaluation Guide

## Scope
- **Source file:** `exception_error_handling.h`
- **Goal:** Verify tool catches incorrect exception patterns and weak error contracts.

## Test Conditions and Expected Review Comments
- **EH-02:** Silent catch-all that suppresses diagnostics.
- **EH-03:** Throwing raw/non-standard exception types.
- **EH-04:** Using exceptions for normal control flow.
- **EH-05:** `noexcept` function with throwing operations.
- **EH-06:** Incorrect rethrow pattern losing original exception context.
- **EH-07:** Plain integer error codes easily ignored.
- **EH-08:** Constructor failure swallowed, object left invalid.

## Pass Criteria
- Tool should recommend explicit, consistent error contracts (`std::exception`-derived types, `[[nodiscard]]`, structured error handling).
