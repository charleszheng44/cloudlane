# Five independent reviews and resolutions

13 September 2026 · Design v2

Exactly five reviewers were spawned with `fork_turns: none` after the design and preview were ready. They received the four user requirements, artifact paths and an assigned review focus, without earlier conversation or other agents' conclusions. Three ran first; two started as slots became available. Each was instructed not to read earlier proposals, archives or other reviews. Refinements occurred during review; individual reports preserve the snapshot they assessed.

| Reviewer | Independent report | Findings and lead resolution |
|---|---|---|
| Product | [v2-product.md](reviews/v2-product.md) | Added audiobooks/drama chapters; saved video, History and Purchases destinations; resource-specific details/owned edit; FM lifecycle; restored search state; selection actions. |
| Native architecture | [v2-architecture.md](reviews/v2-architecture.md) | Defined isolated session contexts, OpenGL/render teardown proof, typed IDs, thread ownership and bounded work, durable task recovery and deterministic transport regressions. |
| Consumer protocol | [v2-protocol.md](reviews/v2-protocol.md) | Bound buffered media to account generation; separated creation/privacy transitions; marked Together inbound/guest leave unverified; preserved raw rights evidence; specified actual asynchronous import modules. |
| Visual/accessibility | [v2-visual.md](reviews/v2-visual.md) | Restored narrow collection/volume access; scaled player/header; made covered content inert; preserved focus; labeled dialogs; fixed wrapped track actions; split control/divider contrast tokens. |
| Adversarial completeness | [v2-completeness.md](reviews/v2-completeness.md) | Defined media transitions, remote synchronization/conflicts, immutable typed identity and full-release gates. Final comment-target/draft finding fixed through shared typed-target entry and per-target drafts. |

The reviewers found useful changes; this is not a claim of five unconditional approvals. The completeness reviewer approved the revised written design for implementation/protocol proof, with one remaining prototype finding subsequently fixed and checked by the lead. Native Qt, account access and live playback remain untested at this design stage.

## Lead verification

- [preview-checks.json](preview-checks.json): basic navigation/forms, duplicate queue identity/order, focus restoration, search escaping, theme and consumer-login fixture.
- [refinement-checks.json](refinement-checks.json): search/detail/Back state, saved/purchased content, owned edits, selection, comments/drafts, FM exit, narrow volume/content access, covered focus and scaled player containment.
- Viewports include 640×480, 960×720, 1280×800 with 14/18/24 px text. Normal measured track row: 48.75 px. HTML checks verify design behavior only.
- [Home](preview-home.png), [Library](preview-library.png) and [compact layout](preview-narrow.png) captures document the revised preview.

## Remaining implementation gates

Required coverage is retained. Protected offline formats, long-form audio/ownership mappings, podcast publishing, identification, Together receive/guest-leave and certain privacy/attachment/purchase contracts need real consumer-protocol proof. Source existence and prototype controls do not close those gaps. Native rendering, accessible Qt controls, actual login/playback and failure recovery must pass before calling the application complete.
