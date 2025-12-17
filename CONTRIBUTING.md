# Contributing to the Logging Module

- Thank you for your interest in contributing to the Logging Module of the Eclipse SCORE project! 
- This guide will walk you through the necessary steps to get started with development, adhere to our standards, and successfully submit your contributions.

## Eclipse Contributor Agreement (ECA)

Before making any contributions, **you must sign the Eclipse Contributor Agreement (ECA)**. This is a mandatory requirement for all contributors to Eclipse projects.

Sign the ECA here: https://www.eclipse.org/legal/ECA.php

Pull requests from contributors who have not signed the ECA **will not be accepted**.

## Contribution Workflow

To ensure a smooth contribution process, please follow the steps below:

1. **Fork** the repository to your GitHub account.
2. **Create a feature branch** from your fork:
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. **Write clean and modular code**, adhering to:
   - **C++17 standard**
   - **Google C++ Style Guide**
4. **Add tests** for any new functionality.
5. **Ensure all tests pass** before submitting:
   ```bash
   bazel test //...
   ```
6. **Open a Pull Request** from your feature branch to the `main` branch of the upstream repository.
   - Provide a clear title and description of your changes.
   - Reference any related issues.

## Development Setup

To build and test the Logging Module, follow the steps below from the project root:

```bash
# Build all targets using default toolchain
bazel build //...

# Run all unit tests
bazel test //...
```

**Prerequisites:** Install [Bazelisk](https://github.com/bazelbuild/bazelisk) to manage Bazel versions. See [Bazel tutorial](https://bazel.build/tutorials) for details.

## Testing

The Logging Module includes extensive tests. Use the following commands:

### Run All Tests
```bash
bazel test //...
```

### Test Categories
- **Unit Tests**: Unit-level testing
- **Component Tests**: Component-level testing
- **Integration Tests**: Cross-component interactions
- **Safety Tests**: ASIL-B compliance verification

## Additional Resources

For project details, documentation, and support resources, please refer to the main [README.md](README.md).

**Thank you for contributing to the Eclipse SCORE Logging Module!** 
