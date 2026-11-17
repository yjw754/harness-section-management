# Stage 2 Visual Report Design

## Intent

Give developers and reviewers a direct view of a Position Point's positive and negative Section trees, the cross-side mapping edges from Stage 1, and the actual circular placement computed in Stage 2. The report should work for later test cases without hand-editing coordinates or adding a GUI dependency to the C++14 core.

## Goals and boundaries

- Generate one self-contained HTML file for a supplied candidate input, relation list, labels, and solve result. Opening the file locally must not require a server or network.
- Show both side trees with stable Segment identity and depth; show ordinary and explicit same-position mappings distinctly.
- Show the solved circles on each side in the shared Position Point 2D frame, including nested parent boundaries, root anchoring, radius, rotation, and coordinates. Preserve the solver's actual values.
- Selecting a tree node, mapping edge, or circle highlights its counterparts and displays its input and solved parameters. Include fit and zoom controls for large cases.
- If solving fails, keep the trees and mapping visible and display structured failure diagnostics rather than inventing positions.
- Keep visualization code in a demo/reporting target; do not change the production solver's decisions or add UI dependencies to `hrn_section_core`.

The report is a diagnostic artifact. It does not provide editing, save changes back to the Position Point, product integration, or publication.

## Approaches

1. **Static SVG for the current fixture:** simple to inspect, but each new case needs a new ad hoc drawing and offers little help finding a Segment among many circles.
2. **Reusable self-contained HTML report (chosen):** a C++14 exporter consumes real Stage 1 and Stage 2 data, while a small embedded script links selection across trees, mapping, diagrams, and details. It takes more implementation work but serves future cases and can show failed solves.

The user chose the reusable interactive report. The implementation will use standard C++14 and inline SVG/HTML; no web framework, server, or external JavaScript library is needed.

## Report layout and behavior

```text
Position Point summary: center, matrix orientation, solve status
┌──────── Positive tree ────────┬── Mapping edges ──┬──────── Negative tree ────────┐
│ nested Segment labels        │ ordinary / equal  │ nested Segment labels        │
└──────────────────────────────┴────────────────────┴───────────────────────────────┘
┌──────── Positive circles ────┬──────────────────── Negative circles ───────────────┐
│ actual center/radius         │ actual center/radius; shared global X/Y basis       │
└──────────────────────────────┴─────────────────────────────────────────────────────┘
Selected Section: identity, side, parent, input radius, solved radius, angle, 2D/3D center
```

All views use the same stable `Side + Segment` key. A mapping row links its positive and negative endpoints; selecting any endpoint highlights both sides. Same-position edges have a distinct label and verify that their solved centers agree. Nested circles remain individually selectable even when concentric. The diagrams auto-fit their effective Root; zoom controls preserve a readable overview for a 100-Section case.

## Data flow and sample cases

The exporter receives `HrnPositionPointInput`, `HrnCircularHierarchyResult`, relations, and a caller-provided Segment label resolver. It derives parent identities from the Segment topology and uses `centers2d`, `locations`, cloned solved Sections, and diagnostics from the solver. Text and embedded data are escaped before writing HTML.

A demo executable writes two reports under the chosen output directory:

- A solvable five-level bilateral case derived from the Stage 1 structure, with mapping edges and actual Stage 2 circle positions. Its explicit equal-center edges must be feasible.
- A five-level case with the Stage 1 topology and original mapping relations, including both explicit equal-center edges, shown as a failed solve with diagnostics. The visualization input uses algorithm version 1 and enables member auto-resize so its constraint conflict reaches the coupled Stage 2 solver; the original Stage 1 fixture stays unchanged.

The public exporter accepts other cases; the demo cases are examples, not special rendering branches.

## Verification

- C++ tests check hierarchy and mapping identities, numeric circle data, HTML escaping, successful and failed result rendering.
- A CTest demo gate generates both files and checks that they exist and carry the expected success or failure status.
- Open the generated report for a visual check of selection, zoom, mapping highlights, and nested circles. Record any browser inspection limits explicitly.
- Stage 1 and Stage 2 suites continue to pass with `HRN_MAX_STAGE=2`; the visualization target remains optional.

## Workspace

Development continues on `feat/stage2-visualization` in the original `/home/yannn/src/harness-section-management` directory, as requested. The `main` branch is not modified.
