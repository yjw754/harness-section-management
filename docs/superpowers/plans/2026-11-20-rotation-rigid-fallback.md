# Rotation Rigid Fallback Implementation Plan

**Goal:** Keep circular rotation-only edits solved by falling back to a rigid rotation of the last valid bilateral placement.

**Architecture:** The debugger supplies a solved snapshot only when the final layout constraints match it, along with the previous input and effective angles. The C++ solver first tries incremental local placement in a stable reference frame. On failure it reuses the solved geometry and applies the rigid rotation at output.

**Tech stack:** C++14, Embind, JavaScript, Chromium, Python Playwright.

**Spec:** [Rotation rigid fallback](../specs/2026-11-20-rotation-rigid-fallback-design.md)

- [x] Replace the old collision-rollback expectation for a rotation-only edit with a failing browser regression for exact bilateral rotation, diagnostics, no-op recalculation, and a subsequent independent leaf edit.
- [x] Separate baseline input angles from effective angles and wire the active edit delta from the debugger. Verify reused geometry through matching layout inputs, including reverted edits; avoid testing rounded snapshot geometry again in the output frame.
- [x] Add importable fixtures and a WASM matrix across mapping classes, nested levels, roots, members, and representative angle deltas. Verify geometry in both local and fallback modes.
- [x] Run native and browser regressions, and document the behavior and matrix for manual checks.
