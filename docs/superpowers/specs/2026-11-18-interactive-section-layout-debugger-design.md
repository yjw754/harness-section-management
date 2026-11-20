# Interactive Section Layout Debugger Design

## Intent

Provide an internal browser-based debugging tool for constructing circular Position Point section cases and observing the real Stage 2 layout immediately. The tool is for the developer working on this repository: it favors fidelity to the C++ solver and fast case iteration over product-level polish, deployment, persistence, or broad compatibility.

The tool must let the user enter side Root radii, circular Section parameters, parent-child nesting, and cross-side mapping relations; solve the case with the existing C++ implementation; and redraw the two side layouts and diagnostics without rebuilding the demo executable for every case.

## Goals

- Run the existing `HrnPositionPointSolver` in the browser through WebAssembly so interactive results use the same implementation as native Stage 2 tests.
- Edit positive and negative side Root radii and a table of circular member Sections.
- Edit each Section's stable string ID, label, side, radius, parent ID, rotation, spacing tolerance, and auto-resize flag.
- Edit cross-side relations and choose ordinary or same-position semantics; same-position is the default.
- Validate malformed input before solving and show precise field/topology errors.
- Recompute after a 300 ms debounce when the input is valid, with an explicit **计算** button for immediate retry.
- Display the current positive/negative trees, mapping rows, solved circles, selected Section details, solver status, and diagnostics.
- Import and export a versioned JSON case so an interesting case can be saved or later converted into a regression test.
- Use the same shape descriptor for side Roots and member Sections. Keep topology and placement separate from shape-specific dimensions so saved circular cases remain readable when support for more shapes is added.
- Serve the generated static files from a local HTTP server such as `python3 -m http.server`; direct `file://` support is not required.

## Non-goals

- No public hosting, authentication, permissions, database, autosave, collaboration, or product integration.
- No drag-and-drop tree editor, graphical edge editing, undo history, or visual design system.
- No rectangular or elliptical editing, rendering, or solving in the first version, including mixed-shape cases. No Stage 3 editing behavior.
- No guarantee for very large cases; the initial tool runs the solver synchronously on the browser main thread and may pause briefly on expensive inputs.
- No requirement to bundle the HTML, JavaScript, and `.wasm` into one file.
- No JavaScript reimplementation of the layout algorithm.

## Approaches

1. **Rewrite the solver in JavaScript:** the smallest browser deployment, but it creates a second algorithm that can drift in tolerance, diagnostics, and placement order. Rejected.
2. **Run a native C++ HTTP service:** preserves the solver but adds server lifecycle, request parsing, ports, and process management to a tool that otherwise consists of static assets. Reasonable for a larger internal application, but too much operational surface here.
3. **Compile the C++ solver to WebAssembly (chosen):** preserves one source of truth and only requires a local static server. Emscripten and a thin binding layer are new build concerns, but they are isolated from native production targets.

## User interface

```text
┌ Case JSON / Root parameters ─────────┬ Positive solved layout ─────┐
│ positive Root radius                │                              │
│ negative Root radius                │                              │
│ [import] [export] [calculate]       ├ Negative solved layout ─────┤
│                                     │                              │
├ Sections ───────────────────────────┤                              │
│ ID | side | parent | radius | ...   ├ Selected Section ───────────┤
│ [+ Section]                         │ input and solved values      │
├ Relations ──────────────────────────┼ Diagnostics ─────────────────┤
│ positive | negative | kind          │ validation / solver messages │
│ [+ Relation]                        │                              │
└─────────────────────────────────────┴──────────────────────────────┘
```

The input side uses ordinary tables and select controls. Parent choices are limited to Sections on the same side; relation endpoint choices are limited to positive and negative Sections respectively. Deleting a Section also removes relations that reference it and clears direct parent references to it after a confirmation-free internal-tool warning in the diagnostics area. Changing a Section's side is rejected while it has a parent, children, or relation endpoints; the user must remove those references first. This avoids silently moving a subtree or rewriting mappings.

The result side reuses the visual language of the existing Stage 2 report: nested tree indentation, mapping rows, SVG circles, selection highlighting, details, fit, and zoom. A successful solve replaces the diagram atomically. A validation or solve failure keeps the last successful geometry dimmed and places the current errors above it, making it possible to compare a broken edit with the previous valid result. Trees, mapping rows, and input details always describe the current draft; a draft selection highlights stale geometry only when the same stable Section ID existed in the last successful result.

Auto-calculate is enabled by default. Text and numeric changes schedule a solve after 300 ms; structural operations schedule immediately after the table state is consistent. While solving, the status reads `计算中`; the synchronous first version prevents overlapping solves by coalescing pending changes and runs one additional solve if input changed during computation.

## Versioned input model

The browser owns a plain JSON-compatible model. Both side Roots and all member Sections use a `shape` object tagged by `kind`. The first version supports only `circular`. The UI keeps a single radius field without offering a shape selector.

```json
{
  "schemaVersion": 1,
  "modelingTolerance": 1e-9,
  "positionTransform": [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1],
  "roots": {
    "positive": {"shape": {"kind": "circular", "radius": 20}, "rotation": 0, "spacing": 0, "autoResize": false},
    "negative": {"shape": {"kind": "circular", "radius": 20}, "rotation": 0, "spacing": 0, "autoResize": false}
  },
  "sections": [
    {"id": "p-main", "label": "P main", "side": "positive", "parentId": null,
     "shape": {"kind": "circular", "radius": 8}, "rotation": 0, "spacing": 0, "autoResize": false},
    {"id": "n-main", "label": "N main", "side": "negative", "parentId": null,
     "shape": {"kind": "circular", "radius": 8}, "rotation": 0, "spacing": 0, "autoResize": false}
  ],
  "relations": [
    {"positiveId": "p-main", "negativeId": "n-main", "samePosition": true}
  ]
}
```

Solve settings use Stage 2 algorithm version 1 and the existing default candidate/iteration budget in the first version. They are deliberately not exposed until a debugging need demonstrates that editing them is useful. Imported documents with an unknown `schemaVersion` are rejected rather than guessed at.

### Future shape support

Reserve these dimension semantics, consistent with the [Section data model](2026-11-13-harness-position-point-section-data-design.md). The non-circular entries document extension points, not accepted inputs for the first version:

| `shape.kind` | Dimensions in model units | Zero-rotation orientation |
| --- | --- | --- |
| `circular` | `radius` | The boundary is rotation invariant; the local axes still orient children. |
| `elliptical` | `semiMajorAxis`, `semiMinorAxis` (half lengths) | Long axis along local X, short axis along local Y. |
| `rectangular` | `longSide`, `shortSide` (full lengths) | Long edge along local X, short edge along local Y. |

Stable IDs, side membership, parent links, mapping endpoints, rotation, spacing, and auto-resize remain separate from the shape descriptor. When the core supports another shape, the user chooses it and edits its dimensions directly in the page: radius for a circle, long/short sides for a rectangle, or major/minor semi-axes for an ellipse. Changing the dimensions uses the same recalculation flow. JSON import/export is only for saving and reusing cases, not a prerequisite for editing dimensions. Extend native shape construction, the solver path, and SVG drawing without changing the tree or mapping model. Extending the debugger does not itself imply support for mixed-shape layouts. Keep version 1 circular files readable and preserve their dimension semantics. Non-circular automatic sizing modes are deferred.

## C++ adapter and WebAssembly boundary

Add a native-testable adapter under `demo/interactive/`:

- `HrnInteractiveCaseDefinition` and child structs carry strings, numeric options, and relation definitions without browser types.
- `solveInteractiveCase(const HrnInteractiveCaseDefinition&)` validates the definition, owns every `HrnSegment` with stable heap lifetime, attaches parent links, builds `HrnPositionPointInput`, invokes `HrnPositionPointSolver::solveCircular`, and returns a debugger-specific result plus validation errors. The first version checks that every Root and member is circular before invoking the solver.
- The adapter reuses `buildVisualReportModel` internally for the Stage 2 circular path. It maps positional `p0`/`n0` identifiers to stable `sections[].id` references in the debugger result and converts radii to tagged shape descriptors. The existing static report API and its positional IDs remain unchanged.

The Emscripten-only binding file uses `emscripten::val` to read the JavaScript case object into the native definition and to return a JavaScript result object. This avoids adding a JSON library to the C++ project. Browser JSON import/export remains JavaScript's responsibility.

The binding exports one stateless operation, conceptually `solveCase(caseObject) -> resultObject`. Each call reconstructs the small debug graph, so there is no cross-call pointer ownership or stale native state. The result contains:

- `valid`, `solved`, and validation errors;
- status and diagnostic strings; the first version does not transport pointer-backed diagnostic targets, conflict witnesses, or adjustments as structured browser objects;
- input and solved side Root shape descriptors;
- visual nodes with IDs, parents, depth, `inputShape` and `solvedShape` descriptors, and separate input rotation, solved absolute rotation, and center fields; solved fields are absent when no geometry was produced;
- mapping relations with same-position flags;
- candidate and iteration counters when available.

The SVG view uses the solved shape, center, and absolute rotation to draw each Section. Fit uses the bounds of the resulting geometry. The first version implements this for circles only. When a draft fails, the dimmed diagram uses shapes and poses from the last successful snapshot, never dimensions from the failed draft. SVG is for inspection; only the C++ solver decides clearance and containment.

## Build and files

The native build remains unchanged by default. Add an opt-in CMake target guarded by both `HRN_BUILD_INTERACTIVE_DEBUG_TOOL=ON` and the Emscripten toolchain. A normal GCC/Clang configuration that enables the option fails during configuration with a direct instruction to use `emcmake`.

Expected static output:

```text
build-wasm/interactive/
  index.html
  app.<sha256>.css
  app.<sha256>.js
  hrn_interactive_solver.<sha256>.js
  hrn_interactive_solver.<sha256>.wasm
```

Example workflow:

```bash
emcmake cmake -S . -B build-wasm -DHRN_MAX_STAGE=2 -DHRN_BUILD_TESTS=OFF -DHRN_BUILD_INTERACTIVE_DEBUG_TOOL=ON
cmake --build build-wasm --parallel
python3 demo/interactive/serve_debugger.py
```

The local server keeps `http://127.0.0.1:8002/` stable and never caches the entry HTML. The build publishes new content-addressed assets before updating the HTML, so ordinary reloads load the matching new JavaScript and WASM without clearing the cache or changing ports.

Emscripten is a developer prerequisite, not downloaded by the normal native build. The implementation starts with a compile-and-solve spike using the smallest bilateral case before the full editor is built. The Stage 2 core and portable adapter stay C++14; only the Embind target can require a newer language level.

## Validation and failure behavior

JavaScript provides immediate field feedback, but the C++ adapter repeats all model-level validation because it is the trusted solver boundary:

- finite positive modeling tolerance and a circular radius greater than that tolerance;
- an explicit supported `shape.kind` on every side Root and member Section; missing, unknown, and reserved but unsupported kinds produce a field error before solving. No fallback to a circle or partial solve. A rejected import leaves the current draft unchanged;
- finite transform, rotation, and spacing values;
- rigid position transform;
- unique, non-empty Section IDs;
- parent exists, is on the same side, is not self, and produces no cycle;
- each Section belongs to exactly one side;
- relation endpoints exist on their required sides and endpoint pairs are unique;
- a side change is rejected while the Section participates in parent, child, or relation references;
- Root and Section objects can be constructed with the existing model rules.

Input validation failures return no new geometry. Solver failures return readable text from the Stage 2 diagnostics and no invented positions. Neither failure crashes or reloads the page.

## Verification

- Native unit tests exercise the adapter without Emscripten: empty sides, asymmetric nesting, one-to-one, one-to-many, many-to-one, invalid parent, cycle, wrong-side relation, duplicate ID, and invalid numeric input.
- Verify that a missing or unknown shape kind is rejected, an elliptical Root and a rectangular member are each rejected before solving, and a rejected import preserves the draft and last successful layout.
- A parity test builds a known case through both the existing case builder and the interactive adapter, then compares solved status, node parent IDs, radii, centers, and diagnostics within the existing tolerance.
- An Emscripten smoke test loads the generated module, solves one bilateral case, and verifies the expected centers and mapping relation.
- A Chromium browser test edits radii and nesting, adds a same-position relation, observes a recomputed layout, exports JSON, reimports it, and confirms that circular shape descriptors and solved results round-trip.
- Native Stage 1 and Stage 2 suites continue to pass with the interactive target disabled.

## Implementation boundary

The builder may choose exact DOM organization, CSS values, helper function names, and the Emscripten module packaging flags, provided the generated files and behavior above remain stable. If the compile spike shows that the core cannot be linked without invasive production changes, stop after documenting the incompatibility and reconsider the local native service approach instead of reimplementing solver logic in JavaScript.
