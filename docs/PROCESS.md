# Development Process

This file explains the normal process for making, validating, and releasing changes.

## 1. Plan

- Define the user-visible behavior change first.
- Identify impacted modules (screen, manager, config, app state).
- Keep scope focused per change.

## 2. Implement

- Modify minimal files required.
- Keep behavior local when possible (for example, screen-specific behavior inside screen class).
- Add small compatibility fallbacks when integrating external systems (for example, HA service payload variants).

## 3. Validate

- Compile:

```bash
platformio run --environment esp32dev
```

- Upload when hardware test is needed:

```bash
platformio run --target upload --environment esp32dev
```

- Verify affected user flow on device.

## 4. Document

- Update CHANGELOG.md under Unreleased.
- Update README.md when setup/workflow/features change.
- Capture stable project notes in repository memory or docs.

## 5. Release Prep

- Confirm clean build.
- Confirm key flows still work:
  - boot/splash
  - WiFi and HA setup
  - dashboard controls
  - long-press light controls
- Tag release version and move Unreleased notes to versioned entry.
