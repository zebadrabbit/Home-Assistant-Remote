# Contributing

Thanks for contributing.

## Development Setup

1. Install VS Code and PlatformIO extension.
2. Open the project folder.
3. Build once to verify toolchain:

```bash
platformio run --environment esp32dev
```

## Branching

- main: stable branch
- feature/*: new features
- fix/*: bug fixes
- chore/*: maintenance updates

## Commit Guidance

Use concise commit messages with intent first:

- feat: add white mode fallback payload
- fix: prevent long-press click-through
- docs: update setup workflow

## Pull Request Process

1. Build and verify locally.
2. Update CHANGELOG.md under Unreleased.
3. Open PR with:
   - summary
   - testing notes
   - screenshots or photos for UI changes

## Code Style

- Prefer clear, small functions.
- Keep LVGL object lifecycle explicit.
- Avoid hidden side effects in callbacks.
- Preserve existing naming and file organization conventions.

## Testing Expectations

At minimum for UI-affecting changes:

- Build succeeds for esp32dev.
- Upload succeeds.
- Manual smoke test on device for touched flows.
