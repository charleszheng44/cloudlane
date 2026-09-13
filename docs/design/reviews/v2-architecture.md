# Independent architecture review — v2

Reviewed 13 September 2026. Inputs: `DESIGN.md`, `COVERAGE.md`, and `preview.html`; primary Qt/mpv documentation and the pinned consumer implementation. No prior proposals, archives, other reviews, account credentials, or live account operations were consulted. No native implementation was supplied or tested.

**Verdict: feasible architecture, with changes needed before implementation is treated as fully specified.** C++20/Qt6/QML, Qt Network/OpenSSL, libmpv, PipeWire and MPRIS can support this application without developer enrollment. The artifacts correctly identify the provider and several full-scope capabilities as unproven. This review accepts the stack direction, not a claim that full consumer-service parity or native playback already works. The HTML is explicitly a fixture-based interaction artifact; its simulated successes provide no native integration evidence.

## Prioritized findings

### 1. High — account generations need to isolate transport state, not just delivered results

**Evidence:** DESIGN §4, line 65, specifies account cancellation and separate secret storage; §5 Provider, line 85, specifies a cookie jar and generation-tagged requests/results. It does not define whether an account switch replaces the network/session context or reuses it.

**Problem:** Ignoring an old result does not by itself stop a transport from processing that response's cookies or protocol-session changes. Qt's network objects call the cookie jar when cookies arrive. The pinned reference also contains process-level token and XEAPI session state. Reusing this state across an account switch leaves the design's isolation promise dependent on an unstated ordering rule. This is a design ambiguity, not an observed credential leak. See [Qt cookie handling](https://doc.qt.io/qt-6/qnetworkcookiejar.html) and the [pinned transport](https://raw.githubusercontent.com/NeteaseCloudMusicApiEnhanced/api-enhanced/a8c781fd64faab17fedfd46e0615a2609307f163/util/request.js).

**Fix:** Define an account/session context that owns its cookie jar, authentication state, refresh operations and applicable protocol keys. Retire that context before activating another; late callbacks may affect only the retired context. Define the lifetime of deliberately shared anonymous/device state separately. Add a delayed Set-Cookie/session-refresh test across account switching, beyond testing whether old model results are discarded.

### 2. High — the native video backend and destruction order are deferred past the first feasibility gate

**Evidence:** DESIGN §5 Player, lines 87–89, promises a validated graphics backend, embedded/fullscreen video, background playback and reopening. §6, lines 101–103, puts video in the third slice; the first proof covers account and audio behavior.

**Problem:** The installed libmpv render API offers OpenGL and software rendering, and requires the proper current graphics context and render-context destruction before destroying the player core. A conventional `QQuickFramebufferObject` integration works only with Qt Quick's OpenGL backend. Hiding, closing, rebuilding or moving the video surface therefore needs an explicit lifecycle, even while audio remains alive. “Render-thread synchronization” alone does not resolve this. See [Qt's backend/thread restrictions](https://doc.qt.io/qt-6/qquickframebufferobject.html) and the local primary headers [/usr/include/mpv/render.h](/usr/include/mpv/render.h:49).

**Fix:** Choose the supported Qt graphics backend and embedding method now. Add a small local-file native spike to the first gate: audio/video handoff, fullscreen, surface destruction/recreation, background close/reopen, mixed-DPI movement and final Quit. Specify render-resource ownership and teardown ordering. Do not infer native video feasibility from the preview's placeholder surface.

### 3. Medium — the stated metadata identity omits the resource kind

**Evidence:** DESIGN §4, line 67, defines keys as provider + account + resource ID. COVERAGE includes songs, albums, artists, playlists, users, videos, episodes and comments, with target-type-sensitive mutations.

**Problem:** The stated key assumes an ID identifies one entity across every resource namespace. The consumer interface itself preserves resource type when addressing targets: the pinned comment module constructs a thread identifier from type plus ID. The design provides no global uniqueness guarantee for the raw IDs. A shared cache or navigation map following the stated key can therefore conflate different resource kinds. See the [pinned comment target construction](https://raw.githubusercontent.com/NeteaseCloudMusicApiEnhanced/api-enhanced/a8c781fd64faab17fedfd46e0615a2609307f163/module/comment.js).

**Fix:** Define identity as provider + account/public scope + resource kind + original string ID, or explicitly use separate typed tables whose table identity supplies the kind. Carry this typed identity through navigation, relations, artwork keys and mutations; continue using separate queue-entry IDs for duplicates. Test two different resource kinds with the same raw ID.

### 4. Medium — asynchronous APIs are named, but thread and work ownership remain unspecified

**Evidence:** DESIGN §5 lists typed models, asynchronous Qt networking and SQLite WAL; §7, line 112, promises no GUI-thread network/decoding and bounded memory. No owner is assigned for replies, JSON/crypto work, SQL connections, local scanning or model updates.

**Problem:** Qt's network manager belongs to one thread, and a Qt SQL connection must be accessed from its owning thread. Asynchronous HTTP does not automatically move response decoding, cryptography, artwork processing or database work away from the GUI. A large library plus uploads/scanning can also overwhelm unbounded worker queues even when artwork has an LRU. See [QNetworkAccessManager](https://doc.qt.io/qt-6/qnetworkaccessmanager.html) and [QSqlDatabase](https://doc.qt.io/qt-6/qsqldatabase.html).

**Fix:** Add a short ownership table: GUI-thread models; provider worker with its network/session objects; serialized database owner; bounded decoding/scanning jobs; independent mpv event and render responsibilities. Cross boundaries with immutable results and queued delivery, never worker mutations of QML models. State cancellation and queue limits, and prioritize playback resolution over bulk work. Validate responsiveness with a scan, upload and library paging running together.

### 5. Medium — WAL and temporary files do not define task recovery after process exit

**Evidence:** DESIGN §5 persists tasks in SQLite, finalizes downloads atomically and defaults window close to quitting. §4 handles ambiguous mutation timeouts. COVERAGE requires task resume, draft retention, uploads, social publication and recovery.

**Problem:** There is no specified durable task state or restart reconciliation. If a publish/upload-complete request succeeds remotely and the process exits before recording its acknowledgement, the next run must distinguish “unknown outcome” from “safe to retry.” Likewise, atomic file rename and recording download completion are separate operations. WAL alone does not make the remote operation, filesystem and database one transaction.

**Fix:** Specify a minimal durable task record and transition model, including account binding, operation identity, remote identifiers, temporary/final file ownership and an unknown-outcome state. On startup reconcile uncertain writes before retrying; recover an already-finalized download without replacing an unrelated file. Define bounded shutdown for transfers and room heartbeats. Add process-termination tests around remote acknowledgement and download finalization, not only network-timeout tests.

### 6. Medium — the native port has no defined regression contract for future protocol fixes

**Evidence:** DESIGN §5 selects a selective C++ port at a pinned upstream revision and says fixes ship in tested releases. §6–7 describe functional acceptance, but not reproducible transport comparison. COVERAGE explicitly warns that module filenames do not prove behavior.

**Problem:** The pinned transport depends on bootstrap token/key state, client headers, endpoint-specific encoding and encrypted responses. A successful QR/audio smoke test cannot show that a later OpenSSL or transport change preserves the remaining operations. Keeping the upstream revision pinned provides provenance, but does not detect a porting mismatch or protect previously working endpoint families. See the [pinned request implementation](https://raw.githubusercontent.com/NeteaseCloudMusicApiEnhanced/api-enhanced/a8c781fd64faab17fedfd46e0615a2609307f163/util/request.js) and [crypto implementation](https://raw.githubusercontent.com/NeteaseCloudMusicApiEnhanced/api-enhanced/a8c781fd64faab17fedfd46e0615a2609307f163/util/crypto.js).

**Fix:** Add deterministic encoding/decoding vectors using injected randomness/time, sanitized request/response fixtures, and a per-family map from upstream module plus helper dependencies to the native implementation. Compare with the pinned reference during development; this need not add a Node runtime dependency to the shipped package. Track verification dates and maintain focused live acceptance for critical account/playback behavior. Make these checks part of the promised tested-release path.

## Strengths

- The chosen application boundary is coherent: one process, one media owner, one MPRIS service, system libraries and no public localhost API. The Arch packaging direction fits that boundary and includes appropriate desktop integration.
- Consumer login is explicit and requires no official developer registration. Missing protocol capabilities remain visible release gaps instead of being relabeled as completed features.
- Account and playback generations, unique queue entries, late URL resolution, conservative availability classification, bounded retries and ambiguous-write reconciliation are good foundational requirements.
- Secret Service with session-only fallback, origin-limited authentication, disabled mpv user scripts/configuration, and separation of streaming from downloads address concrete integration concerns.
- Omarchy integration uses normal window behavior and public configuration inputs rather than private shell imports. Theme replacement, font scaling, accessibility and mixed-DPI acceptance are recognized as native work.
- The preview prominently identifies fictional fixtures and simulated controls. The design explicitly refuses to equate HTML checks, upstream endpoints or successful metadata access with native functionality or entitled full playback.
