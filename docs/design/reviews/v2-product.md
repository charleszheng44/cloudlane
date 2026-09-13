# Independent product and information architecture review

Reviewed: `DESIGN.md`, `COVERAGE.md`, and `preview.html`, Design v2, 13 September 2026. No previous proposals, archive documents, or other reviews were read. This is a design review based on document and prototype-source inspection; no account was accessed and no production functionality was tested. A small public-source check used NetEase's publisher-provided App Store description to assess content scope.

**Verdict: revise before full design sign-off.** The four-destination structure is a sound compact foundation, and most core music activities have an explicit home. Ordinary consumer QR login satisfies the stated setup requirement at the design level. However, a few important capabilities lack a destination or a sufficiently specific workflow, and the preview contradicts several written interaction rules. These findings concern design completeness and consistency; they do not treat an unimplemented native client as a defect in a design deliverable.

1. **High — The “all core functions” claim does not account for a major spoken-content category.**

   **Evidence:** `DESIGN.md` §6, line 97, calls the ledger complete; `COVERAGE.md` lines 27–28 specify podcasts and podcast publishing, while line 18 leaves broader purchases under investigation. There is no explicit audiobook/audio-drama entry, chapter-based detail, or route for an already purchased book. NetEase's current publisher description separately promotes audiobooks/audio dramas and podcasts; this is a content distinction in the service, not merely a new visual treatment. [NetEase's App Store description](https://apps.apple.com/cn/app/%E7%BD%91%E6%98%93%E4%BA%91%E9%9F%B3%E4%B9%90-%E6%95%B0%E4%BA%BF%E9%9F%B3%E4%B9%90%E7%95%85%E5%90%AC/id590338362).

   **Impact:** A customer with purchased long-form audio cannot tell whether this desktop design covers their library. “Investigate all purchase categories” records uncertainty without identifying this concrete content journey.

   **Fix:** Add an explicit long-form audio row: discovery/search → book or series → ordered chapters → preview/purchase entitlement → resume/download where permitted. It may reuse the podcast layout while retaining the correct resource type and labels. Identify consumer-protocol feasibility as a release gap if necessary; do not imply that the existing podcast row proves this coverage. No extra top-level navigation is needed.

2. **High — Collection actions are not consistently paired with a retrieval destination.**

   **Evidence:** `COVERAGE.md` line 17 includes collected videos and line 29 includes video saving, but `DESIGN.md` §2's Library row lists likes, playlists, albums/artists, podcasts and history/purchases without saved video. The actual Library selector in `preview.html` line 44 also omits videos. That selector merges “最近 / 已购” into one category and renders an undifferentiated song table; the Home “最近播放 / 查看全部” action at line 42 opens Library without selecting history. The membership action at lines 67 and 74 does not specify a paid-library destination.

   **Impact:** Save → find again and recent listening → resume are incomplete navigation loops. Combining purchase ownership with listening recency makes different account states hard to understand.

   **Fix:** Add saved MV/video within the existing Library category selector. Give history and purchased content distinct subviews with resource types, useful dates, and appropriate empty states. Route Home's recent-listening link directly to history and the account purchase link directly to purchased content. One example of each complete navigation loop is sufficient for the design preview.

3. **High — Shared detail layouts erase distinctions between people and playable resources.**

   **Evidence:** `COVERAGE.md` line 31 specifies user profiles with followers/following, playlists and posts. In `preview.html` line 46, clicking a post author opens `detailKind='artist'`; there is no user-profile detail type. The generic detail renderer at line 45 gives artist, album, playlist, podcast and video the same Play all/collect/comments structure and song rows. At line 67, every detail's overflow offers “编辑歌单”, which invokes the new-playlist form. This contradicts `DESIGN.md` §2, line 34, which requires actions to reflect resource and ownership.

   **Impact:** The design cannot demonstrate following an ordinary user, visiting their playlists, or editing one's own playlist without offering irrelevant operations on unrelated resources. The problem is the represented information hierarchy, even with fictional fixtures.

   **Fix:** Keep shared chrome, but specify type-specific detail bodies and action sets. Add a user profile with posts/playlists/followers, an artist with songs/albums/biography, and episode/video-specific controls. Distinguish owned and collected playlists; edit must show the existing playlist's metadata and an update outcome. Demonstrate one user-profile loop and one owned-playlist edit loop.

4. **Medium — Personal FM is named, but its listening workflow is not defined.**

   **Evidence:** `DESIGN.md` §2 and `COVERAGE.md` line 11 identify FM as a primary Home action. The player specification in §5 describes a persistent manual queue, but never defines whether FM replaces, suspends, or extends that queue, how recommendations replenish, or how entering and leaving FM works. `preview.html` line 67 merely starts an ordinary fixture track; there is no FM mode indicator or exit/return path.

   **Impact:** A principal discovery feature is currently specified only as an entry button. Users cannot predict what happens to an existing queue, and implementation would have to invent the interaction contract.

   **Fix:** Add a short FM state contract covering entry, previous/next behavior, replenishment, like and any supported negative-feedback action, exhaustion/network failure, and returning to the manual queue. Show the active mode in the persistent player or its overflow. Define how repeat/shuffle and Listen together interact with this mode rather than inheriting manual-queue controls automatically.

5. **Medium — Search is not represented as a complete destination with restorable state.**

   **Evidence:** `COVERAGE.md` line 12 promises songs/albums/artists/playlists/users/podcasts/video/lyrics where supported. `DESIGN.md` §2, line 30, requires filters, selection and scroll to survive detail navigation. The prototype's search handler at line 75 exposes four result types and filters only song fixtures; `snap()` at line 38 does not capture the query or search category, and Back at line 72 explicitly clears the query. The click handler has no search-category route.

   **Impact:** Search → detail → Back loses the user's place, and the design does not show how someone finds a person, episode or video. Working service search is not needed to resolve this design question.

   **Fix:** Model search as its own route with query, result type, paging and scroll state. Specify the complete set of supported result types and record unsupported ones as explicit gaps instead of relying only on “where supported”. Add representative non-song result fixtures, and preserve query/category when opening a result and returning.

6. **Medium — Selection is visible, but the bulk-management workflow and activation rule conflict.**

   **Evidence:** `DESIGN.md` §2, line 32, says a single click selects and Enter/double click plays, with Shift/Ctrl selection. `COVERAGE.md` line 16 promises add/remove/reorder and selection handling. In `preview.html` lines 34, 68 and 69, selection is a checkbox state with no selected-item actions, while a single click on the title plays. The playlist create/edit specification also does not establish where bulk removal or playlist reordering is entered; only queue reordering is shown.

   **Impact:** The design adds permanent selection controls without completing the task they enable, and reviewers receive conflicting answers about the most frequent row interaction. This matters for both minimalism and managing the explicitly targeted large libraries.

   **Fix:** Choose and document one activation model, then align the fixture behavior. Define a compact contextual selection bar or menu with selected count, clear selection, add to playlist, queue/download, and ownership-specific remove/reorder actions. Show it on an owned playlist, with partial failure and unavailable-item behavior specified in text. Clarify that removing a playlist entry does not delete a local file.

**What is strong:** Home, Discover, Library and Activity separate everyday intentions cleanly without adding a sidebar destination for every feature. A persistent player and one context panel support compact use. Cloud files, downloads and local folders remain distinct. Comments, messaging, sharing, listening together, creator workflows and ordinary account recovery are acknowledged instead of silently discarded. Theme/font integration and square, restrained chrome are compatible with the intended Omarchy direction at the specification level. Most importantly, the documents consistently distinguish a designed workflow, upstream protocol evidence, and tested account functionality; QR setup includes no developer-registration or partnership requirement.

The fixes above can fit within the current shell. They require more precise routes, resource types and state examples, rather than more permanent controls or a working service connection.
