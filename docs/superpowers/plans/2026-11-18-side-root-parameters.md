# Side Root Parameters Implementation Plan

> **For agentic workers:** Use superpowers:test-driven-development. Leave changes uncommitted for local HTML review.

**Goal:** Expose the existing circular side Root radius, rotation, spacing and automatic resizing in the browser.

**Architecture:** Add fieldsets to the existing panel. Route controls through `editCaseDraft` using `root-field` actions and refresh from the draft on import; no native or JSON schema changes.

**Tech Stack:** HTML, CSS, JS and Playwright using real Stage 2 WASM.

**Spec:** `docs/superpowers/specs/2026-11-18-side-root-parameters-design.md`

### Task 1: Edit side Root parameters

- [x] Add Chromium tests showing that a fixed undersized Root fails, automatic resizing recovers, Root rotation affects the solved angle, and spacing increases the distance. Check side isolation, JSON round trip and invalid spacing recovery.
- [x] Run against the previous built HTML and confirm failure due to missing inputs.
- [x] Implement in `demo/interactive/{index.html,app.css,app.js}` and document usage in `demo/interactive/README.md`. Render both Root field groups on load/import, preserve drafts during edits, and reuse existing solver scheduling.
- [x] Rebuild `build-wasm/interactive`, run new and existing browser suites and Stage 1/2 native test suites, and leave the HTML for review.
