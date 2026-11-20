# Coupled Circular Parent Repair Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox syntax for tracking.

**Goal:** Solve the reported zero-rotation asymmetric case and its side-swapped counterpart without relaxing constraints.

**Architecture:** Preserve the first fast path, try center-only repair on an otherwise pinned ancestor, then try the opposite side's shared seeds on `NoSolutionFound`. Validate with the existing geometry checks before committing a result.

**Tech Stack:** C++14, GoogleTest, Emscripten, Playwright Chromium.

**Spec:** `docs/superpowers/specs/2026-11-19-coupled-parent-repair-design.md`.

Review ruling: retry the opposite seed once after any coupled `NoSolutionFound`, including a fixed boundary violation. Both seeds can have the same sibling spacing yet differ in containment; the retry still uses unchanged constraints and final validation.

## Global Constraints

- No input/schema changes, no merge. Preserve zero-rotation original JSON and existing successful layouts.
- Search is deterministic and bounded. A candidate overlap is not a hard-conflict proof.

## Review Focus

- Verify a fixed side Root still bounds repaired members and a truly equal-center sibling constraint still yields conflict witnesses.
- Verify both seed directions and ensure the browser exercises the original input rather than the manual rotation workaround.

### Task 1: Repair coupled parent placement and hand off the debugger

- [x] Add `CoupledParentsCanMoveWithoutDraggingMappedChildren` to `tests/unit/hrn_circular_transaction_test.cpp` for the radius-20 bilateral baseline and mirrored case, plus `DistinctSharedCentersDoNotProveAConflict` and `RetriesWhenInitialSharedSeedCrossesFixedBoundary`. Add `test_asymmetric_parent_repair_from_original_case` to `tests/browser/interactive_debugger_browser_test.py` using the reported JSON. Watch the old paths fail before repairing them.
- [x] In `src/section/HrnCoupledCircularSolver.cpp`, distinguish a directly shared node from an ancestor that only contains mapped descendants, validate bounded center-only parent candidates including effective auto-radius, and classify unresolved candidate collisions. In `src/section/HrnCircularHierarchySolver.cpp`, retry the other side's shared seed once on `NoSolutionFound`. Do not change independent or successful first-path layouts.
- [x] Run the new native and Chromium tests green; rebuild WASM and verify both native stages and all Chromium suites, including the static report. Review the scoped diff and leave it unmerged for visual review.

Verification: Stage 1 8/8; Stage 2 53/53; Chromium 24/24. Independent review's fixed-boundary retry finding is covered by the new native regression. Minor deferred: simplify the now-single-certainty boundary diagnostic helper in a later cleanup.
