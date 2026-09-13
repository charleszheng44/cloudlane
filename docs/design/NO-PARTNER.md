# NetEase access without a business partnership

Research update · 13 September 2026

There are two practical paths. Business onboarding is not a prerequisite for developing a native personal NetEase application. The remaining distinction is whether individual developer registration is acceptable, and whether the implementation must be officially supported.

| Route | Setup | Official status | Native desktop fit |
|---|---|---|---|
| Official `@music163/ncm-cli` behind Qt/QML | Individual developer enrollment, personal app credentials, QR account login | Official CLI; our graphical frontend would be independently developed | Native UI and system mpv, with a Node CLI dependency |
| Community consumer-protocol backend behind Qt/QML | Normal NetEase account QR login | Unofficial implementation connecting to NetEase's own service | A compiled Rust/C++ provider can avoid Node, Electron and GTK |

## 1. Official personal route

The strongest new evidence is the [NetEase engineering team's CLI article](https://segmentfault.com/a/1190000047685734). It explicitly describes script integration, JSON results, search/recommendations, favorites/history, playlist changes and independent playback control. It also documents Linux playback through mpv. This supports the inference that a Qt frontend can invoke the CLI and consume its data; a terminal interface need not be the user's interface.

The [personal onboarding guide](https://developer.music.163.com/st/developer/document?docId=9504d35aa41a47c6ac9830b2dbf48f94) describes an online application for adult, real-name-verified individual users, followed by app initialization and credentials. It does not require enrolling a company or negotiating a business partnership. It still is a registration/application step, and request quotas apply.

Proposed integration: Qt/QML pages → asynchronous `QProcess` calls to the installed official CLI → JSON model updates; playback remains under the CLI's supported mpv backend. Keep account credentials in the CLI's configuration and avoid shared application keys. Use a single queue/playback owner and coalesce state refreshes. Test command latency and quota consumption before choosing polling intervals.

A Home page could compose recommendations and recent listening; Library could expose supported favorites and playlist operations. Exact consumer-app Home parity, complete library coverage, video, social functions and offline playback are not established. [Package 0.1.7](https://registry.npmjs.org/@music163/ncm-cli/0.1.7) advertises cloud uploads, podcast management and notes, but the older portal FAQ conflicts with some newer features. Discover and test the actual command tree for the enrolled account. CLI availability does not prove all requested functions are supported.

## 2. Normal-account route with no developer registration

The [native Rust NetEase library](https://github.com/gmg137/netease-cloud-music-api) implements QR login, personal playlists, cloud-list access, recommendations, FM, search, lyrics and Home data. Its [request code](https://github.com/gmg137/netease-cloud-music-api/blob/master/src/lib.rs) targets NetEase consumer servers and uses account cookies rather than a developer app ID/private key. Its dependency manifest contains HTTP, cryptography and serialization libraries; GTK belongs to the separate player frontend, not this library.

[SPlayer-Next's QR login implementation](https://github.com/SPlayer-Dev/SPlayer-Next/blob/dev/electron/main/apis/netease/modules/login_qr_check.ts) independently demonstrates the consumer-account flow. Its [playback module](https://github.com/SPlayer-Dev/SPlayer-Next/blob/dev/electron/main/apis/netease/modules/song_url.ts) requests stream URLs using XEAPI and handles returned availability information. Its [request layer](https://github.com/SPlayer-Dev/SPlayer-Next/blob/dev/electron/main/apis/netease/core/request.ts) implements several consumer transports. A business application is absent from this login path.

These are community implementations of consumer interfaces, not the partner OpenAPI. They can support a substantial desktop app, but protocol changes require maintenance and full parity remains unproven. Playback depends on the service's account and catalog entitlements. A suitable native architecture is Qt6/QML → compiled provider → NetEase servers, plus system libmpv/PipeWire, SQLite and MPRIS. Evaluate the Rust provider first and use current [API Enhanced](https://github.com/NeteaseCloudMusicApiEnhanced/api-enhanced) implementations as a coverage reference. Check the selected source files' licenses before copying code.

Qcm remains a native UI reference. Its extracted `ncrequest` project is a generic C++ HTTP library, not a verified standalone NetEase SDK; this investigation does not recommend it as such.

## Decision and verification boundary

If official integration is mandatory, prototype the official CLI behind the planned native UI using personal enrollment. If ordinary account login with no developer enrollment is mandatory, the community native provider is the practical candidate, with its unofficial status made explicit. No business outreach is needed to investigate either path.

This update verifies public documentation and implementation paths. It does not claim successful playback or full feature parity on this user's account. Earlier anonymous probes established metadata and QR initialization, but did not return playable URLs for the tested tracks. The next functional check is QR authorization, personalized Home/Library reads, and playback of an entitled track through the selected provider. No application was installed and no account credentials were accessed during this follow-up.
