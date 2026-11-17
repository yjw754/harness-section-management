# Stage 2 Visual Reports

Build and generate in the original repository directory:

```sh
cmake -S . -B build-visual -DHRN_MAX_STAGE=2 -DHRN_BUILD_TESTS=ON -DHRN_BUILD_VISUAL_REPORT=ON -DHRN_WARNINGS_AS_ERRORS=ON
cmake --build build-visual --parallel
ctest --test-dir build-visual --output-on-failure
build-visual/stage2_visual_report_demo build-visual/reports
```

Open `build-visual/reports/stage2-solved.html` and
`build-visual/reports/stage1-mapping-conflict.html` directly in a browser.
Open `build-visual/reports/stage2-case-matrix-index.html` to review the complete
`0/1/N × 0/1/N` matrix, where the count is the number of top-level member
Sections directly inside each side Root Section. The index links nine standalone
reports covering `0 × 0`, `0 × 1`, `1 × 0`, `0 × N`, `N × 0`, `1 × 1`,
`1 × N`, `N × 1`, and `N × N`.

The new bilateral cases use same-position relations only. They cover one-to-one,
one-to-many, many-to-one, and an `N × N` mixture of all three. In repeated-endpoint
groups, Sections forced to one center on the same side form a decreasing-radius
ancestor-descendant chain, so the circles are contained rather than overlapping
siblings. Empty-side combinations contain no relation because a relation requires
one endpoint on each side. Positive and negative trees deliberately use different
depths and parent groupings.

`build-visual/reports/stage2-nested-pairs.html` shows the unit-test case with
`p0` containing `p1` and `p2`, `n0` containing `n1` and `n2`, and two
same-position relations (`p1`–`n1`, `p2`–`n2`).
`build-visual/reports/stage2-single-mapped-pair.html` uses the same nesting
with only the `p1`–`n1` same-position relation.
`build-visual/reports/stage2-two-parent-pairs.html` shows `p3` containing
`p0` and `p4`, `p0` containing `p1` and `p2`; on the negative side `n3`
contains `n1` and `n0`, while `n0` contains `n2` and `n4`. The three leaf
pairs have same-position relations.
The reports contain inline CSS, SVG, and JavaScript and require no server or network.
Click a tree node, mapping row, or circle to inspect a section and highlight its
mapped peer. Clicking one mapping row highlights only that relation and shows
both endpoint details. Use the diagram's plus, minus, and fit buttons to change scale.

The successful example has five nested positive sections, three negative sections,
one other positive root, ordinary mapping relations, and one feasible same-position
relation. The conflict example uses the same topology and all four original Stage 1
mapping relations. Both examples use the Stage 2 algorithm version 1 and enable
member auto-resize. The second equal-center edge is incompatible with the nested
topology, so the failed report shows diagnostics without circles. The Stage 1
fixture itself is unchanged.

The report API is `buildVisualReportModel` and `renderVisualReportHtml` in
`demo/HrnSectionVisualReport.h`; pass any Position Point candidate, relation list,
`solveCircular` result, and Segment label resolver to generate a new report.
The exporter is built with `HRN_BUILD_VISUAL_REPORT=ON` for Stage 2 or 3 and
is kept outside the production solver library.

Browser interaction regression checks are in
`tests/browser/stage2_visual_report_browser_test.py`. After installing Python
Playwright and its Chromium headless shell, run:

```sh
python3 tests/browser/stage2_visual_report_browser_test.py build-visual/reports
```

Verification on 2026-11-17: `HRN_MAX_STAGE=2` with warnings as errors passed
43 CTest cases, including the expanded demo artifact gate. The generator produced
all nine matrix reports and `stage2-case-matrix-index.html`. Chromium headless
passed five browser checks covering circle focus, tree and mapping selection,
keyboard selection, independent zoom, fit, failure rendering, a narrow dark
viewport, and index-to-mixed-case navigation. The Stage 1 gate separately passed
8 tests, and the earlier AddressSanitizer build passed 40 Stage 1/2 and report
tests. The browser test saves `browser-circle-selected.png` and
`browser-mobile-dark.png` in the report output directory.
