# Contributing to DLT Log Assistant Plugin

Thank you for your interest in contributing to the DLT Log Assistant Plugin!

## Code of Conduct

Please be respectful and professional when contributing. We aim to create a welcoming environment for all contributors.

## How to Contribute

### Reporting Bugs

1. Check existing issues to see if the bug has already been reported
2. If not, create a new issue with:
   - Clear title describing the problem
   - Detailed description including:
     - Steps to reproduce
     - Expected behavior
     - Actual behavior
     - DLT Viewer version
     - Plugin version
   - Screenshots if applicable

### Suggesting Features

1. Check existing feature requests
2. Create a new issue with:
   - Clear title describing the feature
   - Detailed description of the desired functionality
   - Use cases for the feature
   - Any relevant mockups or examples

### Pull Requests

#### Process

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-new-feature`
3. Make your changes, following the coding standards
4. Add tests for new functionality
5. Ensure all tests pass
6. Commit with clear messages: `git commit -m "Add feature: description"`
7. Push to your fork: `git push origin feature/my-new-feature`
8. Create a Pull Request

#### Coding Standards

- Use C++17 or later
- Follow the existing code style (Qt naming conventions)
- Add comments for complex logic
- Keep functions focused and small
- Use meaningful variable and function names
- Run code formatting before committing

#### Commit Message Guidelines

Use clear, descriptive commit messages:
```
type(scope): description

- Added new feature for X
- Fixed bug in Y
- Updated documentation
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`

### Testing Requirements

All new code must include:
- Unit tests for new functionality
- Integration tests where applicable
- Verify existing tests still pass

Run tests with:
```bash
cmake --build . --target dltchatplugin_test
./plugin/dltchatplugin/dltchatplugin_test /path/to/test.dlt
```

### Documentation

- Update README.md for user-facing changes
- Update DOCUMENTATION.md for technical changes
- Add inline comments for complex code
- Update API documentation for interface changes

## Development Setup

### Prerequisites

- CMake 3.16+
- C++17 compiler
- Qt 5.15+ or Qt 6
- DLT Viewer source (for building as plugin)

### Building

```bash
# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --target dltchatplugin

# Run tests
cmake --build . --target dltchatplugin_test
```

## Recognition

Contributors will be acknowledged in the project README.md.

## Contact

For questions or discussions:
- Open an issue for bug reports or feature requests
- Use discussions for general questions

---

Thank you for contributing to make this plugin better!