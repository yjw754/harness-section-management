# Asymmetric Same-Position Case Matrix Design

## Intent

Extend the Stage 2 visual review artifacts with representative Position Point cases that prove circular harness sections remain non-interfering when the positive and negative Segment trees have different shapes. Reviewers must be able to inspect every combination of zero, one, and multiple top-level Sections on the two sides before the Stage 2 work is considered mergeable.

## Goals and boundaries

- Cover the complete `0/1/N × 0/1/N` matrix, where the count is the number of top-level member Sections immediately inside a side's Root Section boundary.
- Use only same-position cross-side relations in the new cases; do not add ordinary relations.
- Cover one-to-one, one-to-many, many-to-one, and mixed same-position relation cardinalities.
- Make the positive and negative Segment trees structurally different whenever both sides contain Sections.
- Keep every report backed by the real Stage 2 solver and preserve its actual circle centers and radii.
- Generate one self-contained HTML report per case plus a self-contained review index linking all new reports.
- Do not change solver behavior, production interfaces, or the existing report cases.

The zero-side cases necessarily contain no cross-side relation because one endpoint side is empty. They still belong in the matrix because empty-side Root handling is part of the requested review surface.

## Approaches

1. Hand-write illustrative HTML. This gives full control over appearance but cannot prove that the shown circles came from Stage 2, so it is rejected.
2. Add the cases to the existing `stage2_visual_report_demo` using `HrnSectionCaseBuilder` and render them through `buildVisualReportModel` and `renderVisualReportHtml` (chosen). This reuses the accepted report and exercises the real solver without touching production code.
3. Build a separate data-driven case executable. This would isolate the matrix but duplicate the existing demo's solve-and-render path for a small review-only extension.

## Case matrix

`N` means at least two top-level Sections. The report filenames and intended coverage are:

| Positive roots | Negative roots | File | Same-position coverage |
|---:|---:|---|---|
| 0 | 0 | `stage2-roots-0-0.html` | none possible |
| 0 | 1 | `stage2-roots-0-1.html` | none possible |
| 1 | 0 | `stage2-roots-1-0.html` | none possible |
| 0 | N | `stage2-roots-0-n.html` | none possible |
| N | 0 | `stage2-roots-n-0.html` | none possible |
| 1 | 1 | `stage2-roots-1-1-one-to-one.html` | one-to-one |
| 1 | N | `stage2-roots-1-n-one-to-many.html` | one-to-many |
| N | 1 | `stage2-roots-n-1-many-to-one.html` | many-to-one |
| N | N | `stage2-roots-n-n-mixed.html` | one-to-one, one-to-many, and many-to-one |

In one-to-many and many-to-one groups, multiple Sections on the same side that are constrained to one center must be ancestors and descendants, not siblings. Their radii decrease along the ancestry chain so the concentric Sections are contained rather than interfering. Extra top-level roots make the side count `N` without joining that same-center group. The mixed `N × N` case uses separate nested groups so relation equivalence does not accidentally force unrelated sibling Sections to overlap.

## Review artifact

`stage2-case-matrix-index.html` lists the nine combinations, their root counts, relation shape, topology summary, expected successful status, and links to the individual reports. Each report remains standalone and supports the existing tree, mapping-row, circle-selection, detail, fit, and zoom interactions.

The existing successful, conflict, nested-pair, single-pair, and two-parent reports remain unchanged and are not listed as substitutes for the new matrix.

## Verification

- Extend the CMake artifact gate to require the index and all nine reports.
- For every report, assert successful solve status and the expected number of top-level tree nodes and same-position mapping rows.
- Assert that no new report contains the ordinary-relation label.
- Assert that the one-to-many, many-to-one, and mixed reports contain the intended repeated endpoint labels.
- Add a browser check that opens the index, follows a mixed-case link, selects a mapping row, and observes both endpoint details without JavaScript errors.
- Run the Stage 2 CTest suite and the real-browser regression suite, then regenerate the review HTML in the repository build output.
