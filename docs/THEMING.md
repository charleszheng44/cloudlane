# System theme integration

Cloudlane follows the active Omarchy theme automatically. It never writes theme files or calls theme-switching commands on the user's behalf.

`src/theme.cpp` reads the current state-directory theme first, then the legacy config-directory theme. Six-digit hex colors in `colors.toml` are accepted. Missing colors are derived from the actual background/foreground so a partial light palette does not inherit dark selection surfaces. Hover and border colors are semantic values; accent text chooses contrasting black or white. Branding artwork in the desktop icon is fixed; interactive app surfaces follow the palette.

Both `[font] base-size` files are read in order: theme, then user shell settings. On every reload, the font size starts at the shell's 12px default before applying overrides, avoiding stale sizes when an override disappears. Values are bounded to 10–32px. The app's `monospace` font alias follows fontconfig; changing the font family may require reopening Qt applications, while palette and base-size changes update live.

A debounced `QFileSystemWatcher` observes palette/config files and existing ancestor directories. Watches are removed and recreated after reload, so atomic file replacement, missing-file creation and an atomic theme symlink swap remain observable. Changes do not reload the QML engine or disturb the player/account.

`tests/theme_test.cpp` exercises light palettes, derived contrast colors, repeated atomic writes, file creation, user-over-theme font precedence, override removal and an atomic dark-to-light symlink swap followed by a write through the new target. Tests use temporary XDG directories, never the user's active theme.

The optional shell widget uses the host's injected bar colors, font and size, so it follows bar-theme changes independently of the desktop app.
