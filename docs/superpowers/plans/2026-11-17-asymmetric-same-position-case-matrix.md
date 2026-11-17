# Asymmetric Same-Position Case Matrix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Generate and verify a complete `0/1/N × 0/1/N` matrix of Stage 2 HTML reports with asymmetric side trees and same-position-only cross-side relations.

**Architecture:** Extend the existing `stage2_visual_report_demo` with review-only case builders and keep all solving and rendering on the existing `HrnPositionPointSolver` and `HrnSectionVisualReport` path. The acceptance gate treats the generated HTML as the observable contract; a small generated index provides the reviewer entry point.

**Tech Stack:** C++14, CMake/CTest, self-contained HTML/SVG/JavaScript, Python Playwright

**Spec:** `docs/superpowers/specs/2026-11-17-asymmetric-same-position-case-matrix-design.md`

## Global Constraints

- `N` means at least two top-level member Sections directly inside a side Root Section.
- New non-empty cross-side cases use only `same_position=true`; ordinary relations are forbidden.
- Empty-side cases contain no relation because a valid relation requires endpoints on both sides.
- Same-center Sections on one side must be ancestor/descendant Sections with decreasing radii, never overlapping siblings.
- Production solver behavior and interfaces, and all existing report cases, remain unchanged.
- Every generated artifact is self-contained and requires no server or network.

## Review Focus

- An empty side must still render a successful report without invented member circles; Task 1 pins all five empty-side combinations.
- Repeated endpoints in one-to-many and many-to-one relations must remain valid and selectable; Tasks 2 and 3 pin both directions.
- Transitive same-position classes must not collapse unrelated siblings onto one center; Task 3 pins separate groups in the mixed case.
- Top-level count must be distinguished from total nested node count; Tasks 1–3 assert both topology markers and mapping counts.
- The index must use relative local links and work when the report directory moves; Task 4 follows a link in a file-based browser test.

---

### Task 1: Empty-side and one-to-one matrix cases

**Files:**
- Modify: `tests/acceptance/verify_stage2_visual_report.cmake`
- Modify: `demo/stage2_visual_report_demo.cpp`

**Interfaces:**
- Consumes: `HrnSectionCaseBuilder`, `writeCase(const std::string&, HrnSectionTestCase, bool)`
- Produces: reports `stage2-roots-0-0.html`, `stage2-roots-0-1.html`, `stage2-roots-1-0.html`, `stage2-roots-0-n.html`, `stage2-roots-n-0.html`, and `stage2-roots-1-1-one-to-one.html`

- [ ] **Step 1: Add failing artifact assertions**

Extend `verify_stage2_visual_report.cmake` to require the six filenames, `排布成功`, their expected member labels, zero mapping rows for the five empty-side reports, exactly one mapping row for `1 × 1`, and no `普通对应` label in any new report.

- [ ] **Step 2: Run the artifact gate and verify it fails**

Run: `ctest --test-dir build-visual -R stage2_visual_report_artifacts --output-on-failure`

Expected: FAIL because the new reports do not exist.

- [ ] **Step 3: Add the six minimal case builders and emit their reports**

In `stage2_visual_report_demo.cpp`, add focused factory functions for the five empty-side combinations and the asymmetric `1 × 1` tree. Give `N` sides two top-level Sections, use nested child labels to make the bilateral tree shapes visibly different, and add exactly one `relate(..., true)` only to the `1 × 1` case. Add all six `writeCase` calls and output paths to `main`.

- [ ] **Step 4: Build and rerun the artifact gate**

Run: `cmake --build build-visual --parallel && ctest --test-dir build-visual -R stage2_visual_report_artifacts --output-on-failure`

Expected: build succeeds and the artifact test passes.

- [ ] **Step 5: Commit**

```bash
git add demo/stage2_visual_report_demo.cpp tests/acceptance/verify_stage2_visual_report.cmake
git commit -m "test: add empty and one-to-one root matrix reports"
```

### Task 2: One-to-many and many-to-one cases

**Files:**
- Modify: `tests/acceptance/verify_stage2_visual_report.cmake`
- Modify: `demo/stage2_visual_report_demo.cpp`

**Interfaces:**
- Consumes: Task 1's report emission pattern
- Produces: `stage2-roots-1-n-one-to-many.html` and `stage2-roots-n-1-many-to-one.html`

- [ ] **Step 1: Add failing relation-cardinality assertions**

Require both files, successful status, exactly two same-position mapping rows per report, one repeated positive endpoint in `1 × N`, one repeated negative endpoint in `N × 1`, at least two top-level nodes on the `N` side, and no `普通对应` label.

- [ ] **Step 2: Run the artifact gate and verify it fails**

Run: `ctest --test-dir build-visual -R stage2_visual_report_artifacts --output-on-failure`

Expected: FAIL because both reports are missing.

- [ ] **Step 3: Implement the two asymmetric nested cases**

Add `makeOneToManyCase()` and `makeManyToOneCase()` to the demo. The repeated-endpoint side has one mapped Section; the opposite mapped endpoints form a parent-child chain with decreasing radii. Add an unrelated top-level Section to make the required side count `N`. Every call to `relate` passes `true`.

- [ ] **Step 4: Build and verify the focused gate**

Run: `cmake --build build-visual --parallel && ctest --test-dir build-visual -R stage2_visual_report_artifacts --output-on-failure`

Expected: PASS with both reports solved.

- [ ] **Step 5: Commit**

```bash
git add demo/stage2_visual_report_demo.cpp tests/acceptance/verify_stage2_visual_report.cmake
git commit -m "test: add one-to-many same-position reports"
```

### Task 3: Mixed N-by-N asymmetric case

**Files:**
- Modify: `tests/acceptance/verify_stage2_visual_report.cmake`
- Modify: `demo/stage2_visual_report_demo.cpp`

**Interfaces:**
- Consumes: Task 2's safe concentric ancestry pattern
- Produces: `stage2-roots-n-n-mixed.html` with independent one-to-one, one-to-many, and many-to-one relation groups

- [ ] **Step 1: Add a failing mixed-case contract**

Require the mixed filename, successful status, multiple top-level nodes on both sides, five same-position mapping rows partitioned as one one-to-one pair, one two-edge one-to-many group, and one two-edge many-to-one group, plus absence of `普通对应`.

- [ ] **Step 2: Run the artifact gate and verify it fails**

Run: `ctest --test-dir build-visual -R stage2_visual_report_artifacts --output-on-failure`

Expected: FAIL because the mixed report is missing.

- [ ] **Step 3: Implement independent mixed relation groups**

Add `makeMixedManyRootCase()`. Use at least three visibly separate top-level groups per side with unequal nesting depths. Keep each repeated-endpoint equivalence class inside its own decreasing-radius ancestry chain so no relation transitively makes siblings concentric. Emit the report from `main`.

- [ ] **Step 4: Build and verify the focused gate**

Run: `cmake --build build-visual --parallel && ctest --test-dir build-visual -R stage2_visual_report_artifacts --output-on-failure`

Expected: PASS and the report contains solved circle geometry.

- [ ] **Step 5: Commit**

```bash
git add demo/stage2_visual_report_demo.cpp tests/acceptance/verify_stage2_visual_report.cmake
git commit -m "test: add mixed same-position root matrix report"
```

### Task 4: Review index and browser traversal

**Files:**
- Modify: `demo/stage2_visual_report_demo.cpp`
- Modify: `tests/acceptance/verify_stage2_visual_report.cmake`
- Modify: `tests/browser/stage2_visual_report_browser_test.py`
- Modify: `docs/verification/stage2-visual-report.md`

**Interfaces:**
- Consumes: all nine report filenames from Tasks 1–3
- Produces: `stage2-case-matrix-index.html` as the reviewer entry point

- [ ] **Step 1: Add failing index and browser checks**

Require the index file, all nine relative links, the `0/1/N × 0/1/N` heading, and rows identifying one-to-one, one-to-many, many-to-one, and mixed coverage. Add `test_case_matrix_index_opens_mixed_report` to open the index, follow the mixed relative link, click a mapping row, and assert two endpoint detail blocks and no page error.

- [ ] **Step 2: Run focused checks and verify failure**

Run: `ctest --test-dir build-visual -R stage2_visual_report_artifacts --output-on-failure`

Expected: FAIL because the index is missing.

- [ ] **Step 3: Generate the self-contained review index**

Add `writeCaseMatrixIndex(const std::string&) -> bool` in the demo. Write semantic UTF-8 HTML with inline CSS, one row per matrix cell, relative links, root counts, relation cardinality, topology summary, and expected successful solve status. Call it from `main` after the reports are written.

- [ ] **Step 4: Document the review path**

Update `stage2-visual-report.md` with the index filename, the full matrix definition, the same-position-only rule, and the ancestry requirement that makes one-to-many and many-to-one non-interfering.

- [ ] **Step 5: Run artifact and browser verification**

Run: `cmake --build build-visual --parallel && ctest --test-dir build-visual -R stage2_visual_report_artifacts --output-on-failure`

Run: `python3 tests/browser/stage2_visual_report_browser_test.py build-visual/reports`

Expected: all artifact and browser tests pass without JavaScript errors.

- [ ] **Step 6: Commit**

```bash
git add demo/stage2_visual_report_demo.cpp tests/acceptance/verify_stage2_visual_report.cmake tests/browser/stage2_visual_report_browser_test.py docs/verification/stage2-visual-report.md
git commit -m "demo: add stage two case matrix review index"
```

### Task 5: Full verification and review artifacts

**Files:**
- Modify: `docs/verification/stage2-visual-report.md`

**Interfaces:**
- Consumes: complete matrix generator and tests
- Produces: verified HTML files under `build-visual/reports/` and recorded verification result

- [ ] **Step 1: Configure the supported Stage 2 build**

Run: `cmake -S . -B build-visual -DHRN_MAX_STAGE=2 -DHRN_BUILD_TESTS=ON -DHRN_BUILD_VISUAL_REPORT=ON -DHRN_WARNINGS_AS_ERRORS=ON`

Expected: configure succeeds.

- [ ] **Step 2: Build and run the complete CTest suite**

Run: `cmake --build build-visual --parallel && ctest --test-dir build-visual --output-on-failure`

Expected: build and all tests pass.

- [ ] **Step 3: Regenerate the final reports**

Run: `build-visual/stage2_visual_report_demo build-visual/reports`

Expected: the command lists the nine matrix reports and `stage2-case-matrix-index.html`, and every file exists.

- [ ] **Step 4: Run the complete browser suite**

Run: `python3 tests/browser/stage2_visual_report_browser_test.py build-visual/reports`

Expected: all browser checks pass without page errors.

- [ ] **Step 5: Record the exact verification result**

Update `docs/verification/stage2-visual-report.md` with the current CTest and browser counts and the date `2026-11-17`.

- [ ] **Step 6: Commit**

```bash
git add docs/verification/stage2-visual-report.md
git commit -m "docs: verify asymmetric same-position case matrix"
```

