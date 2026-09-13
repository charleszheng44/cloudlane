# Independent adversarial completeness review

Reviewed the current v2 DESIGN.md, COVERAGE.md, preview.html, its loaded preview-refinements.js, and the supplied home/narrow screenshots. No previous proposals, archives, or other reviewers' files were read. Final inspected snapshot: DESIGN.md 15:16:17, COVERAGE.md 15:14:56, preview files 15:18:22 on 13 September 2026, America/Los_Angeles. The screenshots predate the latest preview refinements and were used only for the overall visual direction.

**Verdict: approve the written design for the planned implementation/protocol proof; one medium-priority preview correction remains.** It meets the four stated requirements at design level. This does not certify that the required consumer protocols work: the ledger explicitly separates inspected sources, native implementation work, and unresolved feasibility, and full release is now gated on every required row.

## Remaining finding

1. **P2 — Track comments can show the wrong resource, and switching targets discards drafts.** DESIGN §2 requires drafts keyed by account and typed target and says a composer cannot change its target. In preview.html's `data.trackAction` handler, choosing a track's Comments directly calls `openPanel('comments')`. This bypasses the target assignment in preview-refinements.js:44. For example, with 夜航 current, open 城市边缘's overflow → 评论: the panel still identifies 夜航. The refinement also keeps only one `commentDraft` and clears it on a different target, rather than preserving the per-resource draft described in the design. These are misleading interaction results even in a fixture-only prototype; no live account is needed to reproduce them. **Fix:** make every comment entry point call one function with the selected resource kind and ID, render that target, and store/retrieve drafts by that key. Verify current-track, other-track, collection, and post entry points, then switch away/back with an unsent draft and change playback while composing.

## Concerns resolved during this review

- **Resource identity:** DESIGN §4 now includes resource kind and account/public scope in metadata keys and carries that identity through artwork, relationships, navigation, and mutations. The earlier provider/account/ID tuple was underspecified across the many resource families.
- **Core playback transitions:** DESIGN §2 now defines ordinary row Play, collection replacement, removing the playing entry, music/video/spoken-session handoff, together restoration, and speed reset, alongside the detailed FM lifecycle. “One media owner” alone did not establish these user-visible outcomes.
- **Remote library synchronization:** DESIGN §2 now supplies cache freshness, activation/explicit/reconnect refresh rules, targeted invalidation, and comparison with phone edits before saving. Synchronization has concrete behavior and test cases without adding permanent UI controls.
- **Full scope and estimates:** DESIGN §6 now prohibits describing a release as full/complete with a failing required ledger row. Earlier builds are labeled, documenting a gap does not waive it, and the estimate must be revisited after feasibility proofs. The conditional multi-month estimate is therefore planning input rather than a delivery guarantee.
- **Preview alignment:** The loaded refinement file adds typed detail actions, separate saved video/books/history/purchases, expanded search routes, FM return, bulk selection, volume/mute overflow, and panel focus handling. The previous narrow layout's missing volume path is corrected. Player refresh no longer reconstructs and empties the comment textarea.

## Strengths

- Four destinations, one persistent player, and one optional panel provide a compact hierarchy while the coverage ledger retains substantial music, library, cloud/offline, spoken audio, video, and social workflows.
- Ordinary phone-approved QR login is the concrete account path. The architecture introduces no developer registration, app credential setup, business partnership, or public API server requirement.
- C++/Qt/QML with system libmpv, compositor-owned borders, opaque theme-derived surfaces, system fonts, and explicit narrow/font-scaling behavior is a coherent native Omarchy direction.
- Availability and quality are evidence-derived; preview URLs, unexplained missing URLs, account switching, ambiguous writes, and protected offline playback receive explicit handling. The design avoids treating an upstream filename or successful membership lookup as functionality proof.
- Unknown books/purchases, protected downloads, publishing, identification, and together receive named feasibility/release conditions. An unbuilt client is appropriately treated as implementation work, not as a defect in this design deliverable.

This review used source inspection and visual inspection; it did not run authenticated requests or native-client tests.
