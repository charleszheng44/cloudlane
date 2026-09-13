# Native implementation status

Development preview, 13 September 2026. This is the working C++/QML implementation of the reviewed design. It is not a full-parity release. The feature ledger remains the release gate.

## Implemented paths

- Qt6 Quick UI with the active Omarchy palette, monospace font, font size, square controls, keyboard navigation, and compact four-route navigation.
- Native WEAPI, EAPI, and XEAPI consumer transport using Qt Network and OpenSSL. No Node service or Electron runtime.
- Consumer QR login with refresh, expiry, cancellation, bounded Secret Service access, and account-generation invalidation.
- Search by resource type; playlist, album, artist and podcast views; discovery; account library categories; paged results and complete playlist track-ID retrieval.
- libmpv playback with service-granted URLs, quality/trial handling, duplicate-safe queue entries, reordering/removal, pending-queue shuffle, repeat, private FM, and podcast resume/speed/sleep timer.
- Synchronized lyrics and translation; resource-bound comment reading/likes; consumer feed, notice and conversation reading.
- User-initiated likes, collection operations, private/public playlist creation, rename, delete and adding songs. Writes are not exercised automatically against the user's account.
- Native OpenGL video surface; MPRIS playback interface and single-instance activation; local-file metadata scans through TagLib on a worker.

## Still required before a daily-player release

Authenticated login, entitled streaming, full account-library response validation and write acceptance tests require an ordinary NetEase account. Anonymous transport success is not authenticated feature verification. Native video and desktop integration checks are recorded separately as they run.

## Full-release work remaining

Durable downloads and upload/task recovery; full cloud editing/upload; folder indexing and offline cache; richer playlist editing/import; word-level lyrics; listening-history reporting; social composition and inbox sending; native comments challenge handling; native identification; audiobooks and purchase mapping; creator publishing; Together receive/synchronization/guest leave; protected offline formats; background/notification preferences; accessibility and failure-recovery audits.

No third-party stream substitution, subscription bypass, region spoofing or consumer-cookie extraction from other applications is included.
