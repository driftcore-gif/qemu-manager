# Contributing to QEMU Manager

Thank you for your interest in contributing!

## Getting Started

1. Fork the repository on GitHub
2. Clone your fork:
   ```bash
   git clone https://github.com/driftcore-gif/qemu-manager.git
   cd qemu-manager
   ```
3. Set up the upstream remote:
   ```bash
   git remote add upstream https://github.com/driftcore-gif/qemu-manager.git
   ```

## Code Style

- C++17 standard
- 4-space indentation
- PascalCase for classes, camelCase for methods, snake_case for variables
- Each source file must have a corresponding header in `include/`
- No external dependencies beyond GTK4 (no libvirt, no nlohmann_json)

## Building for Development

```bash
meson setup build --buildtype=debug
cd build && ninja
```

## Submitting Changes

- One feature or fix per pull request
- Write a clear PR title and description
- Reference related issues with `Fixes #123`
- All PRs must build without errors on GTK4 >= 4.10

## Reporting Bugs

Use the GitHub Issues tab with the *Bug Report* template.

## Requesting Features

Use the GitHub Issues tab with the *Feature Request* template.

## Code of Conduct

Be respectful. No harassment, discrimination, or toxic behavior.
