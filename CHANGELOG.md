# Changelog

All notable changes to this project are documented in this file.

The format is based on Keep a Changelog and semantic-style categories.

## [Unreleased]

### Added

- Idle screen dim/screensaver after inactivity with touch-to-wake behavior.
- Light color badge dot on dashboard tiles when rgb_color is present.
- LVGL colorwheel support enabled and integrated into light popup.
- Light popup initializes from Home Assistant entity attributes on open (brightness, color, white temp).
- Dashboard ticker auto-collapses to a bell indicator after 60s of no new messages.
- Tapping the ticker opens a scrollable notifications popup with recent entries.
- Climate entities are now shown on the dashboard with compact temp/humidity tile stats.
- Climate detail popup shows mode, HVAC action, current temp, humidity, and setpoint.

### Changed

- Light popup redesigned with explicit Color and White modes.
- Apply button now applies changes without dismissing the popup.
- Cancel button renamed to Close.
- Dashboard tile ON/OFF text removed for binary light/switch entities.
- Long-press interaction handling improved to avoid accidental toggles.
- Firmware size optimization applied via linker dead-code elimination and LVGL feature trimming.
- Climate tile name behavior updated to single-line truncation and repositioned to avoid stat overlap.

### Fixed

- White mode apply now uses compatibility payload fallback for broader HA integrations.
- Popup control overlap issues adjusted through layout updates.
- Settings modal now reflects persisted configuration values correctly.
- Climate tile label wrapping/overlap issue that caused name text to collide with temp/humidity lines.

## [0.1.0] - 2026-05-31

### Added

- Initial ESP32 + LVGL application structure.
- WiFi onboarding and Home Assistant connectivity flow.
- Dashboard control tiles and status bar.
- Home Assistant setup portal and persistent configuration storage.
