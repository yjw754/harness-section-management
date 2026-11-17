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
The reports contain inline CSS, SVG, and JavaScript and require no server or network.
Click a tree node, mapping row, or circle to inspect a section and highlight its
mapped peer. Use the diagram's plus, minus, and fit buttons to change scale.

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

Verification on 2026-11-17: `HRN_MAX_STAGE=2` with warnings as errors passed
41 CTest cases, including the demo artifact gate. The Stage 1 gate separately
passed 8 tests. No browser executable was
available in the workspace, so visual interaction was not inspected in a browser.
An AddressSanitizer build passed 40 Stage 1/2 and report tests.
