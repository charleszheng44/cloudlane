# Contributing to Cloudlane

Start with the [implementation status](docs/IMPLEMENTATION.md) and [feature ledger](docs/design/COVERAGE.md). The native app is a development preview; the HTML design files are an earlier prototype, not implemented feature evidence.

Use system Qt6/C++20 and the qmake build in the README. Keep production code free of bundled web runtimes or local API servers. Use the Omarchy palette through `Backend.theme`, system monospace fonts, vector controls, plain English interface text and original-language service content.

Keep API wire values separate from translated UI labels. For example, NetEase's Chinese category parameters must retain their exact values. Protocol behavior must be supported by a pinned primary reference and tests, then clearly distinguished from real-account acceptance. Respect service-granted availability, quality, trials and downloads.

Run `scripts/check.sh` before a pull request. Changes to playback or views also need the native graphical test. Theme changes need the isolated palette tests; plugin changes need `omarchy plugin validate .` and `scripts/check-plugin.sh` in a graphical session. Describe the concrete behavior change, validation and remaining limits. Keep external writes and account acceptance explicit and user initiated.

Use fixture data for screenshots and bug reports. Do not commit cookies, QR URLs, signed media URLs, private playlists, account databases or personal screenshots. `scripts/screenshot.sh` renders the actual app with a test-only backend and original fixture art. Review the image before replacing `preview.png`.

New original contributions are provided under GPL-3.0-or-later. Preserve existing third-party notices and document new dependencies in `THIRD_PARTY.md`. Do not add proprietary client code, subscription bypasses, alternate-source unlocking or registration credentials.

Repository layout: `src/` native services and player; `qml/` desktop interface; `plugin/` optional shell widget; `tests/` fixture and native tests; `packaging/` desktop integration; `docs/` design, limits and release guidance.
