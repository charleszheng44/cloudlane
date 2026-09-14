# Third-party components

Consumer transport in `src/crypto.cpp` is derived from API Enhanced revision `a8c781fd64faab17fedfd46e0615a2609307f163`, particularly `util/crypto.js`. Its MIT notice is preserved in [third_party/API-Enhanced-LICENSE](third_party/API-Enhanced-LICENSE). These are public consumer-protocol constants, not developer credentials or user account secrets.

The application links to distribution Qt6, OpenSSL, zlib, libmpv, libsecret, TagLib and libqrencode. Their respective upstream/distribution license terms apply. No proprietary NetEase CLI code, alternative-source unlocking module or account credential is included.

Cloudlane's original work is GPL-3.0-or-later. System libmpv is GPL-2.0-or-later by default (LGPL builds are possible); see [mpv Copyright](https://github.com/mpv-player/mpv/blob/master/Copyright). The application uses GPL-3.0-or-later to accommodate the distribution's GPL-enabled multimedia stack. Qt6 modules used here are available under LGPL-3.0/GPL terms; libsecret and libqrencode use LGPL terms, TagLib LGPL/MPL, OpenSSL Apache-2.0, and zlib the zlib license. Distribution builds and transitive multimedia components may carry additional notices. Binary distributors must preserve applicable notices and provide corresponding source as required by their build's licenses.

`tests/fixtures/art/*.svg`, the Cloudlane icon and the vector player controls are original project artwork. `preview.png` is a capture of the native interface with fictional metadata and those original assets. It contains no NetEase cover-art assets or private account data. Historical HTML design documents are prototypes and are not bundled in the app.
