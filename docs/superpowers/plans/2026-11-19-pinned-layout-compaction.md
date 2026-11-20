# Pinned Layout Compaction Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make mapped layouts such as `deranged.json` compact and regular after shared centers move.

**Architecture:** Extend circular rule requests with fixed placements and an optional parent radius limit; use them once to repack free sibling subtrees around moved mapped obstacles before coupled collision repair.

**Tech Stack:** C++14, CMake/CTest, Emscripten, Playwright.

**Spec:** `docs/superpowers/specs/2026-11-19-pinned-layout-compaction-design.md`

## Global Constraints

- Preserve the existing feature checkout and all earlier uncommitted work.
- Maintain all seven unconstrained rule reference vectors, same-position overlap, parent containment, spacing, rotations, and singleton-follow behavior.
- Fixed placements and failed repack attempts cannot mutate input or committed geometry.

## Review Focus

- Compacting around moved fixed circles must work symmetrically on either side.
- Fixed-boundary candidates must be filtered while searching, not only rejected afterward.
- Same-position duplicate centers must use one maximum-radius obstacle.
- Free subtrees must move rigidly; mapped descendants must stay fixed.
- Equal-envelope scores should favor compact positions without changing unconstrained vectors.

### Task 1: Fixed-placement rule candidates

**Files:** `include/hrn/section/HrnRuleBasedLayoutSolver.h`, `src/section/HrnCircularRuleBasedLayoutSolver.cpp`, `tests/unit/hrn_circular_rule_layout_test.cpp`.

**Interfaces:** `HrnCircularLayoutRequest` gains fixed placements and optional radius limit; unchanged `solve()` returns fixed and free placements.

- [x] Write tests for two fixed centers (-5,0)/(8,0), radius-8 free circle tangent to both, secondary compactness scoring, fixed containment/spacing, invalid and budget inputs.
- [x] Observe failures, implement constrained requests, verify new tests and frozen vectors.

### Task 2: Coupled repack and original case regression

**Files:** `src/section/HrnCoupledCircularSolver.cpp`, `tests/unit/hrn_circular_transaction_test.cpp`, `tests/browser/interactive_debugger_smoke_test.py`, `demo/interactive/README.md`, `docs/verification/stage2.md`.

**Interfaces:** Consume Task 1 requests from moved pinned/free families, preserving maximum-radius equivalence envelopes and atomic subtree moves.

- [x] Add and run failing `deranged` native regression in both side directions; check envelope <=15, mapped tangency, free contacts, clearance, containment and repeatability.
- [x] Implement one optional family repack in parent-local coordinates; preserve old positions on incomplete repacks.
- [x] Exercise original JSON through rebuilt WASM; compare before/after diagrams and document results.
- [x] Run native suites, original browser suites, diff checks and independent read-only code review.
