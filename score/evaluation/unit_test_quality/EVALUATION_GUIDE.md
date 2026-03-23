# Unit-Test Quality Evaluation Guide

## Scope
- **Source under test:** `unit_test_quality.h`
- **Test file under review:** `unit_test_quality_test.cpp`
- **Goal:** Verify tool catches missing/weak unit-test conditions and poor mocking practices.

## Test Conditions and Expected Review Comments
- **UT-01:** Test has no assertions.
- **UT-02:** Only happy path covered; key boundaries/failure paths missing.
- **UT-03:** Magic numbers in test logic reduce intent clarity.
- **UT-04:** Exception contract not tested (`EXPECT_THROW` missing).
- **UT-05:** Copy-paste duplicated test body.
- **UT-06:** Incomplete edge-case partitions for string split behavior.
- **UT-07:** Poorly named tests (`Test1`/`Test2`) reduce diagnosability.
- **UT-08:** Core overwrite-on-full behavior of circular buffer not validated.
- **UT-09:** Empty-pop exception path not tested.
- **UT-10:** White-box fragile assertion style without justification.
- **UT-11:** `ON_CALL` used without `EXPECT_CALL` verification.
- **UT-12:** Test couples to internal state instead of externally observable behavior.
- **UT-13:** Mocked dependency failure path never exercised.

## Pass Criteria
- Tool should flag missing negative tests, boundary-value gaps, and weak interaction verification in mocks.
