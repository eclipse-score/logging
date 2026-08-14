
# Logging

This module provides components (`mw::log`, `datarouter`, `score_log_bridge`) for
safety-critical logging (Diagnostic Log and Trace) in embedded automotive systems.

See the [module documentation](docs/index.rst) for the full overview, repository layout, and component
architecture.

---

## 🚀 Getting Started

### 2️⃣ Build & Run Tests

See the [Quick Start](docs/index.rst) section of the module documentation for the exact `bazel build`/
`bazel test` commands (including the platform `--config` and `--test_tag_filters` needed to select
unit, component, or QNX/QEMU integration tests).

> TIP: For a single release-artifact target, provide a `:release_artifacts` filegroup per module
> (e.g. `bazel build //score/<module_name>:release_artifacts`) — final decision is up to the module
> maintainer.

---

## 📖 Documentation

Documentation lives in `docs/` and is built with `docs-as-code` (Sphinx). Build it locally with:

```sh
bazel build //:docs
```

See the [module documentation](docs/index.rst) for the rendered content.

---

## ✅ Quality

See the [Quality](docs/index.rst) section of the module documentation for the existing tooling and the
KPIs each one gates in CI (static analysis/linters, sanitizers, coverage, test tiers, traceability, license compliance).

---

## IDE support

### Rust

Use `scripts/generate_rust_analyzer_support.sh` to generate `rust_analyzer` settings that will let VS Code work.
