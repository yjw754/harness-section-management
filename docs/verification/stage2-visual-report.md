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
`build-visual/reports/stage2-nested-pairs.html` shows the unit-test case with
`p0` containing `p1` and `p2`, `n0` containing `n1` and `n2`, and two
same-position relations (`p1`–`n1`, `p2`–`n2`).
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
41 CTest cases, including the demo artifact gate. The Stage 1 gate separately
passed 8 tests. An AddressSanitizer build passed 40 Stage 1/2 and report tests.
Chromium headless passed four browser checks covering circle focus, tree and
mapping selection, keyboard selection, independent zoom, fit, failure rendering,
and a narrow dark viewport. The browser test saves `browser-circle-selected.png`
and `browser-mobile-dark.png` in the report output directory.
