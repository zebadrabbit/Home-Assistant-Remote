# Changelog

All notable changes to this project are documented in this file.

The format is based on Keep a Changelog and semantic-style categories.

## [Unreleased]

### Added

- Idle screen dim/screensaver after inactivity with touch-to-wake behavior.
- Light color badge dot on dashboard tiles when rgb_color is present.
- LVGL colorwheel support enabled and integrated into light popup.
- Light popup initializes from Home Assistant entity attributes on open (brightness, color, white temp).

### Changed

- Light popup redesigned with explicit Color and White modes.
- Apply button now applies changes without dismissing the popup.
- Cancel button renamed to Close.
- Dashboard tile ON/OFF text removed for binary light/switch entities.
- Long-press interaction handling improved to avoid accidental toggles.

### Fixed

- White mode apply now uses compatibility payload fallback for broader HA integrations.
- Popup control overlap issues adjusted through layout updates.
- Settings modal now reflects persisted configuration values correctly.

## [0.1.0] - 2026-05-31

### Added

- Initial ESP32 + LVGL application structure.
- WiFi onboarding and Home Assistant connectivity flow.
- Dashboard control tiles and status bar.
- Home Assistant setup portal and persistent configuration storage.
