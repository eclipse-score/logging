# S-CORE Logging Integration Example

This directory contains a minimal example demonstrating how to use S-CORE Logging
(`score_logging`) as an external Bazel module dependency.

## Purpose

This example serves as an integration test in CI to verify that S-CORE Logging:
- Can be consumed as a Bzlmod module by external projects
- Has correct transitive dependency declarations (including `score_baselibs`)
- Exposes only intended public targets

## Building the Example

```bash
cd examples/integration
bazel build //...
```

## Modifying the Example

To add additional S-CORE Logging features to test:

1. Add dependencies to the `cc_binary` target in `BUILD`
2. Include headers in `main.cpp`
3. Run `bazel build //...` to verify

Example adding the file backend:
```python
# BUILD
deps = [
    "@score_logging//score/mw/log",
    "@score_logging//score/mw/log/backend:file",
],
```

## Configuration

Required flags in `.bazelrc` (these mirror what the host workspace sets):

- `--@score_baselibs//score/json:base_library=nlohmann`
- `--@score_baselibs//score/memory/shared/flags:use_typedshmd=False`
- `--@score_baselibs//score/mw/log/flags:KRemote_Logging=False`
- `--@score_logging//score/datarouter/build_configuration_flags:persistent_logging=False`
- `--@score_logging//score/datarouter/build_configuration_flags:persistent_config_feature_enabled=False`
- `--@score_logging//score/datarouter/build_configuration_flags:enable_nonverbose_dlt=False`
- `--@score_logging//score/datarouter/build_configuration_flags:enable_dynamic_configuration=False`
- `--@score_logging//score/datarouter/build_configuration_flags:file_transfer=False`
- `--@score_logging//score/datarouter/build_configuration_flags:use_local_vlan=True`

These must be set by consumers to properly configure S-CORE Logging.
