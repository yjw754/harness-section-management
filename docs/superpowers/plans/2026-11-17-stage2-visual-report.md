# Stage 2 Visual Report Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Generate reusable, offline HTML reports that show bilateral Stage 1 Section trees and mapping relations together with real Stage 2 circular layouts or failure diagnostics.

**Architecture:** A C++14 report model extracts stable Segment identities, topology, input parameters, solved geometry, and diagnostics from existing APIs. A renderer writes escaped data into a self-contained HTML document whose embedded SVG and small script coordinate tree, mapping, circle, and detail selection. A demo executable builds a successful and a conflicting five-level case and writes both reports.

**Tech Stack:** C++14, CMake 3.16, GoogleTest 1.16, HTML/CSS/SVG/vanilla JavaScript; no runtime network dependency.

**Spec:** `docs/superpowers/specs/2026-11-17-stage2-visual-report-design.md`

## Global Constraints

- Preserve the C++14 core and existing `HRN_MAX_STAGE=1/2` gates; the visualization target requires Stage 2 and is optional.
- Report positions and radii only from `HrnPositionPointSolver::solveCircular`; never generate visual placement coordinates separately.
- Use stable `Side + Segment` IDs for tree entries, mapping endpoints, circles, and selection.
- Escape all caller-provided labels and text before embedding in HTML or JavaScript.
- A failed solve shows no fabricated circles and retains input trees, mapping, and structured diagnostics.
- Reports open from `file://` without network access or a local server.

## Review Focus

- A label containing `<`, `&`, quotes, or `</script>` must render as text and not execute markup or script (Task 2).
- The same Segment present on both sides must have separate side-scoped visual identities (Task 1).
- A mapping endpoint omitted from one side must be reported as invalid input, not linked to another row (Task 1).
- Deep concentric children must remain selectable from the tree even when the smaller SVG circle is covered (Task 2).
- A failed solve must not display stale solved centers from a prior report (Task 2).

## Files

| Path | Responsibility |
| --- | --- |
| `demo/HrnSectionVisualReport.h/.cpp` | Generic report API, model extraction, HTML escaping, SVG/HTML document rendering |
| `demo/stage2_visual_report_demo.cpp` | Builds the two documented cases and writes reports |
| `tests/unit/hrn_section_visual_report_test.cpp` | Data identity, escaping, geometry, and failure rendering tests |
| `tests/acceptance/verify_stage2_visual_report.cmake` | Demo artifact gate |
| `CMakeLists.txt` | Optional Stage 2 report target and tests |
| `docs/verification/stage2-visual-report.md` | Commands, report paths, browser inspection and results |

---

### Task 1: Extract a stable bilateral report model

**Files:** Create `demo/HrnSectionVisualReport.h/.cpp`; Test `tests/unit/hrn_section_visual_report_test.cpp`.

**Interfaces:** `HrnVisualReportModel buildVisualReportModel(const HrnPositionPointInput&, const std::vector<HrnSegmentSideRelation>&, const HrnCircularHierarchyResult&, const std::function<std::string(const HrnSegment&)>& labels)` returns nodes with side-scoped IDs, parent IDs, input and solved values, relations, and diagnostics; `valid/error` indicate malformed mapping endpoints.

- [ ] Write tests: five-level parent IDs, same Segment on two sides has distinct IDs, ordinary/equal mapping metadata, solved center/radius equals solver output, missing endpoint rejected, failure model has no circles.
- [ ] Run `cmake -S . -B build-visual-dev -DHRN_MAX_STAGE=2 -DHRN_BUILD_TESTS=ON -DHRN_BUILD_DEMO=ON && cmake --build build-visual-dev --target hrn_section_visual_report_test --parallel`; expect compile failure before implementation.
- [ ] Implement model extraction without mutating input or solver result.
- [ ] Run `ctest --test-dir build-visual-dev -R hrn_section_visual_report_test --output-on-failure`; expect PASS; commit `feat: add section visual report model`.

### Task 2: Render an interactive self-contained report

**Files:** Modify `demo/HrnSectionVisualReport.h/.cpp`; Test `tests/unit/hrn_section_visual_report_test.cpp`.

**Interfaces:** `std::string renderVisualReportHtml(const HrnVisualReportModel&)` returns a complete HTML document. Selection keys match Task 1 IDs; fit/zoom and mapping highlights run locally.

- [ ] Write tests: escaped hostile labels, visible tree and mapping labels, actual circle center/radius attributes, no circle SVG after failure, details/zoom/select hooks exist, no external URLs.
- [ ] Run target test; expect FAIL on missing renderer.
- [ ] Implement semantic HTML, inline CSS/SVG, and a small vanilla JS selection and zoom controller; tree buttons remain a fallback when circles overlap.
- [ ] Run unit test and inspect generated HTML syntax; expect PASS; commit `feat: render interactive section report`.

### Task 3: Build demo reports and verification gate

**Files:** Create `demo/stage2_visual_report_demo.cpp`, `tests/acceptance/stage2_visual_report_acceptance_test.cpp`, `docs/verification/stage2-visual-report.md`; Modify `CMakeLists.txt`.

**Interfaces:** `stage2_visual_report_demo <output-directory>` writes `stage2-solved.html` and `stage1-mapping-conflict.html`. The first solves a five-level bilateral variant; the second uses Stage 1 topology and mapping with Stage 2 algorithm version 1 and member auto-resize, and records its explicit equal-center conflict.

- [ ] Write acceptance test that runs the demo into a test output directory and checks both files, status, tree labels, mapping type, and solved versus failed circle presence.
- [ ] Run the test target; expect FAIL before demo/CMake wiring.
- [ ] Implement case builder, file output, CMake target, and gate; document build/open commands and the difference between the two case inputs.
- [ ] Run `cmake -S . -B build-visual -DHRN_MAX_STAGE=2 -DHRN_BUILD_TESTS=ON -DHRN_BUILD_DEMO=ON -DHRN_WARNINGS_AS_ERRORS=ON && cmake --build build-visual --parallel && ctest --test-dir build-visual --output-on-failure`; expect all Stage 1/2 and report tests PASS.
- [ ] Run ASan build and `git diff --check`; visually inspect both reports if a browser preview is available; document any unverified interaction; commit `test: verify stage two visual reports`.
