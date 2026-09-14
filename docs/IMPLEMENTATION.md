# Native implementation status

Development preview, 13 September 2026. A working C++/QML implementation is included. This is not a full-parity release; the reviewed [feature ledger](design/COVERAGE.md) remains the release gate.

## Implemented and locally verified

- Qt6 Quick UI, live Omarchy palette/font-size integration, system monospace font, square controls, four-route navigation, responsive cards and virtualized track lists.
- Native WEAPI/EAPI/XEAPI transport using Qt Network and OpenSSL, without a Node service or Electron runtime. Real anonymous search and QR-challenge retrieval pass.
- Consumer QR approval recovered and persisted in Secret Service; saved-account restoration, playlist listing, daily recommendations and a full-track playback grant verified with an ordinary account. Native background login polling and retry paths have regression tests.
- libmpv audio and embedded OpenGL video, play/pause/seek, service-grant quality/trial handling, duplicate-safe queue entries, move/remove, future-queue shuffle and repeat.
- SQLite cache and local-file library, worker scans through TagLib, file/folder import, copied NetEase resource links and desktop file opening.
- Download engine with separate download grants, durable tasks, three-transfer limit, pause/retry, Range handling, size/checksum verification and atomic final-file rename. The engine is fixture-tested; service grants require account verification.
- Session D-Bus MPRIS controls, typed metadata and single-instance activation; desktop launcher packaging.
- Automated action tests for request cancellation, account/UI invalidation, QR refresh, correct comment identity, trial bounds and paused FM/video queue restoration.

## Implemented paths needing an ordinary NetEase account

| Area | Native path |
|---|---|
| Login/session | QR refresh/expiry/cancel/confirm, bounded Secret Service storage, logout, cookie/key-context retirement on account change |
| Home/Discover | Recommended playlists, daily songs, FM, charts, new songs/albums, recommended podcasts and MV browsing |
| Search/details | Songs, albums, artists, playlists, podcasts, MVs, users; paged results; full playlist track-ID retrieval |
| Library | Playlists, likes, albums, artists, podcasts, saved videos, recent plays, purchased albums, cloud tracks, downloads and local music |
| Collections/playlists | User-initiated likes and album/artist/radio collection; private/public playlist creation, rename, delete and adding songs |
| Lyrics/comments | Line synchronization, translation, offset and seek; resource-bound reading/sorting/paging and comment likes |
| Spoken/video | Episode playback, resume, speed and sleep timer; online MV/video URL resolution and return to paused music |
| Activity/inbox | Feed and notification reading; conversation list and history reading |

Source-backed request paths are not proof that each current account response works. No real account writes were performed automatically. The app has no fake music or simulated account responses; the HTML design preview and test fixtures remain separate.

## Full-release work remaining

- Remaining real-account acceptance: a fresh approval through the final QR flow, online stream decoding, download grants, all library payloads, ownership checks and write acceptance. Saved-session restoration, playlists, daily recommendations and one playback grant pass.
- Cloud metadata editing/matching/delete/upload, upload credentials and task recovery.
- Playlist descriptions/covers/privacy transitions/reorder/import and durable reconciliation for ambiguous writes.
- Word-level/romanized lyrics, opt-in listening-history reporting, richer artist/discovery filters and complete local-library search/removal workflows.
- Social composition, replies/deletion/reporting and native challenge/token handling; inbox sending and unread state.
- Audiobooks, purchase-category mapping, podcast creator tools, native identification and Together receive/synchronization/guest leave.
- Protected offline formats, download removal/cancel/resume recovery across changed grants, scalable scans beyond the current 20,000-file batch, background/notification preferences and mini-player.
- Large-font/accessibility audit, long-session protocol-key rotation, suspend/device switching, disk-full/network-loss testing and a complete release audit.

No third-party stream substitution, subscription bypass, region spoofing or cookie extraction from other applications is included. Normal consumer login requires no developer registration; the protocol implementation is unofficial.
