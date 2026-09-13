# Feature coverage ledger

Design v2 · 13 September 2026

Every row is in full core scope. **Source** = community implementation inspected; **Local** = app-owned behavior; **Investigate** = incomplete protocol evidence. None means verified live functionality for this account. Names reference [API Enhanced modules](https://github.com/NeteaseCloudMusicApiEnhanced/api-enhanced/tree/main/module), pinned by the [source inventory](evidence/consumer-module-inventory.json). A filename alone is not proof of working request/response behavior.

| Feature | Native entry/workflow | Evidence | Acceptance |
|---|---|---|---|
| QR login | Account sheet: scan/confirm, expiry, refresh/cancel | Source: login_qr_key/create/check, login_status | Ordinary account, no app credentials; challenge recovery |
| Accounts/session | Identity, switch, logout/relogin, restart | Source: user_account/detail, login_refresh/logout; Local Secret Service | No stale account data; secrets cleared on logout |
| Home | Daily songs, FM, playlists, resume, refresh | Source: homepage_block_page, recommend_songs/resource, personal_fm | Personalized account results, ordered, signed-out/stale state accurate |
| Search | Global songs/albums/artists/playlists/users/podcasts/video/lyrics where supported | Source: cloudsearch/search | Typed results, paging, cancellation and IME |
| Discover | Charts, genres, playlists, new songs/albums | Source: toplist/detail, top_song, album_new, top_playlist | Filters/detail/back restore correctly |
| Artist/album | Tracks, discography, biography, videos, follow/save | Source: artist_detail/songs/album/mv, album, artist_sub/album_sub | Correct IDs, paging and collection state |
| Playlist library | Created/collected lists, complete contents and subscribers | Source: user_playlist, playlist_detail/track_all/subscribers | No truncation; stable order/duplicates |
| Playlist edits | Create, add/remove/reorder, name/description/cover/privacy/delete | Source: playlist_create/update/tracks/order_update/cover_update/privacy/delete; Investigate public → private transition | Creation privacy and edit transitions tested separately; unsupported reverse transition never reports success |
| Playlist import | Library → Import; preview, task creation/progress, partial/unmatched results, reconciliation | Source: playlist_import_name_task_create, playlist_import_task_status | Persist task ID; no duplicate create after timeout; accepted task distinct from imported library |
| Likes/collections | Player heart; liked songs, albums/artists/videos | Source: likelist/like, album_sublist, artist_sublist, mv_sublist | Acknowledged changes synchronize views |
| History/purchases | Library selector, recent tracks, stats, paid items | Source: user_record, record_recent_song, digitalAlbum_purchased, vip_info; Investigate all purchase categories | Accurate account/dates; entitlement not inferred from membership |
| Queue/playback | Play/pause/seek/skip, shuffle/repeat, reorder/remove/Play next | Local + Source: song_url_v1 | Duplicate entries, actual queue order and shuffle history correct |
| Quality/rights | Actual versus requested quality; full/preview/unavailable | Source: raw song_url_v1/detail/privilege fields and independent SPlayer XEAPI; exclude lossy check_music decoder | Preserve null and non-null trial results, unknown reasons, expiry and actual quality |
| Lyrics | Line/word sync, translation/romanization, offset, seek/copy | Source: lyric/lyric_new; Local UI | Follows player time, optional translations, missing lyrics recover |
| Listening history | Progress reporting/settings | Source: scrobble/scrobble_v1 | Actual progress only; no duplicate reports on seek/restart |
| Cloud management | Library Cloud: quota, search, metadata/match, delete | Source: user_cloud/detail/del, cloud_match/import | Paged list, correct identity/match, explicit delete |
| Cloud upload | File picker, progress, cancel/retry and per-file outcomes | Source: cloud, cloud_upload_token/complete | Scoped credentials, duplicate/limit/failure/cancel cases |
| Download/offline | Queue, quality/path, progress, resume/retry/cancel, completed playback | Source: song_download_url_v1; Investigate protected offline formats | Download grant distinct from streaming; integrity, entitlement and offline proof |
| Local music | Files/folders, scan, tags/art, search/play, missing-file recovery | Local Qt/SQLite/libmpv and metadata adapter | Unicode/common formats/removable drives; explicit file deletion |
| Podcasts | Discover, subscribe, episodes, resume/played state, speed/sleep | Source: dj_catelist/recommend/detail/program/sub/sublist; Local speed/resume | Correct episode identity; no music-speed carryover |
| Audiobooks / audio dramas | Discover/Search → series → ordered chapters; Library purchases/subscriptions; narrator, preview, entitlement, resume/download | Investigate resource/consumer endpoint mappings; do not equate podcast APIs with book support | Purchased series retrievable; chapter order/resume/rights correct; named full-release gap until verified |
| Podcast publishing | Account Creator tools: create/edit, episode upload/draft/publish/status | Investigate consumer write/upload contracts | Eligible normal account, end-to-end explicit publish/edit |
| MV/video | Discover/detail: browse, embed/fullscreen, quality, save/comments | Source: mv_all/detail/url/sub, video_group/detail/url/sub | Native rendering/audio handoff, quality/restrictions, supplied captions |
| Saved video retrieval | Library → saved videos → MV/video detail | Source: mv_sublist; Investigate non-MV video collection listing | Each Save has a corresponding native retrieval path; missing generic-video listing reported |
| Comments | Context: sorting, threads, write/reply/like/delete own/report | Source: comment_new/floor/add/reply/like/delete/report | Correct target type/ownership; explicit submit; visible errors |
| Profiles/follows | Activity/profile: followers/following, playlists/posts, follow toggle | Source: user_detail/follows/followeds/event, follow | Paging/privacy and state correct |
| Feed/posts | Activity: feed, share music, compose text/images, forward/delete own | Source: event, share_resource, event_forward/del; Investigate attachment breadth | Draft retention, upload failures, explicit publish and privacy |
| Inbox/notices | Header: unread counts, notices/mentions, conversations, Send | Source: msg_notices/comments/private/private_history, send_text/playlists/song | Explicit send, accurate read state, bounded refresh |
| Listen together | Player sheet: create/join/invite, people, roles/sync, leave/end | Source: listentogether_room_create/check, accept, heatbeat, play_command/sync_list_command/end; Investigate inbound events and guest-only leave | Two accounts/unequal rights, version/sequence/drift/reconnect, own media resolution, guest leaves without ending host room |
| Links/sharing | Resource menu/desktop handler: copy/import valid NetEase links | Local URI handling; Source share_resource for posting | No automatic publish/join; malformed/unrelated links rejected |
| Account/membership | Profile, entitlements, paid library, daily sign-in | Source: user_update/vip_info/daily_signin; consumer web checkout | No developer account; external checkout not called native |
| Identification | Search action: file or explicitly enabled microphone, match results | Investigate audio_match fingerprint/native implementation | Consent/cancel, native fingerprint proof, no continuous recording |
| Desktop | MPRIS/media keys, notifications, single instance, background setting, mini-player | Local Qt DBus/libmpv/Wayland | One owner; Raise/Quit, suspend/output changes, no Super conflicts |
| Theme/accessibility | Palette/font/scale, focus, keyboard, CJK, accessible roles | Local shell tokens and Qt | Replacement/high text scale/contrast/native accessibility checks |
| Recovery | Retry/stale timestamps, task failures, relogin, pending writes | Local typed provider/task state | Offline/timeout/throttle/keyring/disk-full/stale response cases |

API Enhanced is a reference; the app does not run its Node server. Selectively port and attribute necessary code. Alternative-source unlocking and region-spoofing modules are not part of the provider. Only service-returned, account-appropriate content belongs in this app.

The initial native provider and player now exist; see [implementation status](../IMPLEMENTATION.md). Full-release gaps still include authenticated account/playback verification, protected offline playback, podcast authoring, identification and broader attachment/purchase categories. Workflows are designed; implementation and feasibility are not claimed complete. Ancillary mobile live/commerce/promotional services are outside the desktop music scope defined in DESIGN.md.
