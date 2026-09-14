# Security and privacy

Cloudlane is an independent development preview using an unofficial NetEase consumer-protocol implementation. Only the current main branch receives fixes. Ordinary NetEase account rights, region availability and service behavior still apply.

## Report a vulnerability

Do not post credentials or exploitable account details in public issues. When the repository is public, use GitHub's private **Report a vulnerability** form if enabled. If private reporting is unavailable, open an issue asking the maintainer for a private reporting channel, without exploit details or personal data. A public launch checklist includes enabling private vulnerability reporting.

## Access and data

The native app connects to NetEase for browsing, QR authentication, account functions and service-granted media. Images and media are fetched from service-provided URLs. Imported local files are read for playback and metadata. Downloads are written to the standard music directory. It opens no local HTTP server and includes no telemetry or crash-upload service.

Account cookies are stored using the desktop Secret Service, scoped by the account identifier. If the keyring cannot save them, the interface reports that the session will not persist. Settings and SQLite contain device/account identifiers, UI choices, cache, local file paths and download tasks; these are private app data, not safe bug-report attachments. Signed playback/download URLs are not persisted in the download database.

The English rename deliberately retains the original Qt storage identity `Yunjian/yunjian` and Secret Service schema `io.github.charleszheng44.Yunjian`, so existing users keep their login and library:

- Settings: `$XDG_CONFIG_HOME/Yunjian/yunjian.conf` (default `~/.config/Yunjian/yunjian.conf`).
- SQLite: `$XDG_DATA_HOME/Yunjian/yunjian/library.sqlite3` (default `~/.local/share/Yunjian/yunjian/library.sqlite3`).
- New downloads use the `Cloudlane` subdirectory under the standard music directory; existing stored download paths remain valid.

Sign out in the app to remove the saved current account and stop account-bound playback. Uninstalling preserves app data and downloads. To erase them, sign out, close the app, then explicitly remove its settings/database paths and any downloads you no longer want.

## Shell widget

Omarchy plugins run unsandboxed in the shell. This widget launches the fixed command `cloudlane` using an argument array and uses session D-Bus MPRIS to discover/control `org.mpris.MediaPlayer2.cloudlane`. Now-playing metadata is visible through the normal desktop media interface. The widget does not access Secret Service or account APIs and does not execute downloaded code, elevate privileges or change shell configuration.

The user-local app installer builds this checkout and writes only to the chosen prefix. It does not fetch external build scripts or modify Omarchy-managed files. System dependencies can be installed separately through the distribution package manager. Plugin removal and app removal are separate operations.
