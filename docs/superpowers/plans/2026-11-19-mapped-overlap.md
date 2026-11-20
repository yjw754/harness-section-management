# Mapped Overlap Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Support intentional same-side overlap induced by cross-side same-position mappings.

**Architecture:** Build validated same-position equivalence classes once, share them between initial hierarchy packing and coupled geometry validation. Pack same-class siblings using their largest radius and preserve individual members.

**Tech Stack:** C++14, CMake/CTest, Emscripten, Python Playwright.

**Spec:** `docs/superpowers/specs/2026-11-19-mapped-overlap-design.md`

## Global Constraints

- Preserve existing uncommitted work in this checkout; do not commit unrelated changes.
- No external API/schema changes. Only explicit same-position edges grant overlap permission.
- Parent containment, unrelated sibling spacing, geometry ownership, and transaction rollback remain mandatory.

## Review Focus

- Fixed Root smaller than the sum but large enough for the maximum radius must succeed.
- Nested siblings and transitive mapping components receive the same exemption as top-level members.
- Ordinary edges cannot exempt unrelated collisions.
- Group envelopes must use the largest member regardless of insertion order.
- Removing an exemption or exceeding a fixed parent must preserve formal state on failure.

### Task 1: Shared-center packing and validation

**Files:** `src/section/HrnSamePositionClasses.h`, `src/section/HrnCircularHierarchySolver.cpp`, `src/section/HrnCoupledCircularSolver.cpp`, `tests/unit/hrn_circular_transaction_test.cpp`, `tests/acceptance/stage2_acceptance_test.cpp`.

**Interfaces:** Consume `HrnPositionPointInput` and relation vectors; produce unchanged `solveCircular` results with intentional overlap accepted.

- [x] Add failing tests for both mapping directions, fixed/auto Root, unequal radii, nested siblings and transitive components, plus ordinary-edge and containment/rollback controls.
- [x] Build and run targeted tests; confirm missing overlap support causes failure.
- [x] Share validated equivalence classes; pack same-class siblings with a maximum-radius envelope; exempt only same-class sibling pairs in coupled repair and final validation.
- [x] Update obsolete conflict acceptance to assert successful shared centers; run full native CTest.

### Task 2: Importable cases and integration

**Files:** `docs/verification/cases/`, `tests/browser/interactive_debugger_smoke_test.py`, `demo/stage2_visual_report_demo.cpp`, `tests/acceptance/verify_stage2_visual_report.cmake`, `docs/verification/stage2.md`, `docs/verification/stage2-visual-report.md`, `demo/interactive/README.md`, existing data-model spec.

**Interfaces:** Consume Task 1 solver through unchanged native/WASM adapters and JSON schema v1.

- [x] Add 1-to-2 and unequal-radius/unrelated-member JSON cases alongside original N_1; exercise imported files through WASM and assert shared centers and unrelated clearance.
- [x] Replace obsolete demo failure topology with a genuine fixed-boundary failure, preserving a useful failure report and browser assertions.
- [x] Update behavior documentation; rebuild native and WASM; run native suites, WASM/browser tests, and diff checks.
- [x] Review the scoped changes and record verification results.
