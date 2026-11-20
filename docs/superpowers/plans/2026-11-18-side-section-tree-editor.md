# Side Section Tree Editor Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans and test-driven-development. Keep changes uncommitted for local HTML review.

**Goal:** Replace the mixed section table with positive and negative editable hierarchies.

**Architecture:** Keep `editCaseDraft` and the existing solver boundary. Add an optional parent to the add-section action, and share selection between the editor and result views. Keep active parameter controls mounted during in-place edits.

**Tech Stack:** Existing HTML/CSS, JavaScript and Chromium/Playwright with WASM.

**Spec:** `docs/superpowers/specs/2026-11-18-side-section-tree-editor-design.md`

## Global Constraints

Do not change Stage 2 solving, circular case schema, cross-side relation workflow or the existing static reports.

## Review Focus

- Both sides can be empty and rebuilt independently; newly added descendants inherit their parent's side.
- Reparenting and deleting preserve the existing policy; a temporarily invalid topology remains editable.
- Stable ID references, focus on rename, and selection via solver graphics keep working.

### Task 1: Edit members in two trees

- [x] Add Chromium coverage in `tests/browser/interactive_debugger_browser_test.py` for deep asymmetric creation, dropping to an empty side, selection and reparenting. Run against the built prior HTML and see the missing tree fail.
- [x] Replace mixed member UI in `demo/interactive/{index.html,app.js,app.css}` with two trees and one property editor; adapt old test selectors to exercise selecting nodes before editing with the same assertions.
- [x] Update `demo/interactive/README.md`. Rebuild generated HTML, rerun interactive and static browser suites plus native Stage 1/2, and leave the result for review.

Review follow-up: preserve result-tree controls and diagnostic panel height during ID commits, focus newly created nodes for keyboard users, and keep tables from widening the page at narrow viewports. These interactions were reproduced as failing browser tests before the fixes.
