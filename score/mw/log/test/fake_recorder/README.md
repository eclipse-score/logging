# FakeRecorder – How to use in Unit Tests

`FakeRecorder` is a test implementation of `score::mw::log::Recorder` used in unit tests.
When enabled, logging output is routed to **stdout**, which allows tests to validate log output using GTest's `testing::internal::CaptureStdout()`.

## Enable FakeRecorder in your test target (BUILD)

Add this dependency to your unit test target:

```bzl
deps = [
  "//score/mw/log/test/fake_recorder_environment:auto_register_fake_recorder_env",
]
```

This target installs `FakeRecorder` via a **GTest Environment**, so most tests do not need to include any FakeRecorder headers or manually configure the recorder.

**Recommended**: Do not include FakeRecorder headers in tests unless you explicitly need direct access to FakeRecorder APIs.

## Typical test pattern: capture stdout and assert content

```cpp
#include "score/mw/log/logging.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(FakeRecorderUsage, CapturesLogOutput)
{
    testing::internal::CaptureStdout();

    // Any mw::log statement will be routed to stdout when FakeRecorder is installed
    score::mw::log::LogFatal("test") << "HELLO";

    const std::string out = testing::internal::GetCapturedStdout();
    // Note: always call GetCapturedStdout() inside the test to finalize the capture.
    EXPECT_THAT(out, ::testing::HasSubstr("HELLO"));
}
```

## What gets validated?

The most reliable assertion is that the captured stdout **contains the text you streamed** into the logger (e.g. `"HELLO"`, `"EVENT"`, `"INVALID"`).

Exact formatting of severity / context / metadata is not guaranteed by FakeRecorder and should not be asserted unless the logger itself guarantees it.

## Thread-safety notes

FakeRecorder serializes:

* internal state updates using `Synchronized<State>`
* stdout writes using a global mutex to avoid interleaved output

When multiple threads log concurrently:

* stdout writes are guarded to minimize interleaving
* ordering between threads may be nondeterministic (expected)

## Real example in the repository

`score/mw/com/impl/service_element_type_test.cpp` demonstrates the intended usage:

* the test target depends on `auto_register_fake_recorder_env`
* the test captures stdout
* the test asserts that expected text appears in captured output
