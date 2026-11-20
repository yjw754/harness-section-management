# Singleton Same-Position Follow Implementation Plan

> **For agentic workers:** Use test-driven development and verify the native and WASM builds. Leave the working tree uncommitted for review.

**Goal:** Keep the multi-root side's rule-based layout when its only explicit same-position edge leads to an opposite side containing one member.

**Architecture:** Select the multi-root endpoint as the shared-center seed in the existing coupled circular solver. The other endpoint may translate; all existing geometric and final equal-center checks still run. Ordinary relations do not count as explicit edges.

**Tech Stack:** C++14, GoogleTest, Emscripten and Playwright.

**Spec:** Section 6.4.1 in `docs/superpowers/specs/2026-11-13-harness-position-point-section-data-design.md` (confirmed Stage 2 exception).

## Review Focus

- Mirror the case, use all three endpoints, and preserve world-space mapping after a non-identity transform.
- Do not activate the exception for two explicit edges, neither side having multiple top-level members, or a single-root side with descendants.
- Use the review JSON fixture in the browser and verify the tight fixed Root remains unchanged.

### Task 1: Implement the bounded exception

- [x] Add native transaction regressions and a real Chromium test using three unit circles packed inside radius 2.16. Check equal centers and unchanged multi-root positions; watch the old coupled solver fail.
- [x] Modify `src/section/HrnCoupledCircularSolver.cpp` without changing the validation/commit path, solver version, JSON or browser layout. Update spec and debugger usage.
- [x] Rebuild Stage 1, Stage 2, static reports and WASM. Verify native and browser suites and hand off the review fixture and generated HTML.

The existing candidate-space diagnostic test formerly relied on a singleton being anchored. Its opposite-side root now has a child so the same test still verifies an actual search failure outside the exception, including its heuristic adjustment.
