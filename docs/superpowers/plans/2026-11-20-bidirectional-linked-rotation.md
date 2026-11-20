# Bidirectional Linked Rotation Implementation Plan

**Goal:** An interactive rotation edit moves the edited subtree and its same-position counterparts by the change in angle, from the last valid placement.

**Architecture:** The debugger supplies a transient placement baseline for rotation-only edits. The circular solver rotates prior parent-relative offsets, chooses shared centers from the affected side, and validates them without repacking unrelated siblings. Regular solves and the case JSON format stay unchanged.

**Tech stack:** C++14 circular solver, Embind, JavaScript debugger, Chromium regression tests.

- [x] Reproduce the missing negative-side movement in Chromium with `docs/verification/same_side.json`; assert the exact incremental movement in both directions.
- [x] Add optional rotation placement to the circular solve and pass unmodified case settings, prior centers and angles, and the last edited side from the debugger. Preserve the previous layout on failure, and reset the baseline on import or non-rotation layout edits.
- [x] Verify side-root activation, ordinary correspondences, collision rollback, manual recalculation, and ordinary solves. Document how to try the case and the session-only baseline.

## Follow-up: Asymmetric Nested Parents

- [x] Reproduce `partial_nested.json` failing on edits to either side Root despite matching rotated centers.
- [x] Reuse coupled parent repair for unmapped auto-resizing ancestors of displaced mapped members. Repair bottom-up, keeping mapped centers and ordinary siblings fixed. Cover both stale nested radii and a repairable fixed-boundary violation.
- [x] Verify both incremental Root edits preserve the shared centers, spacing, and fixed Root radius. Keep rollback when no allowed repair is found.

## Follow-up: Unmapped Top-Level Sibling

- [x] Reproduce `same_side_rot.json` importing successfully but failing after either side Root changes because the opposite free sibling overlaps the moved endpoint.
- [x] Verify bidirectional incremental rotation, clearance, and fixed boundaries in the original case. Preserve an already valid follower layout.
- [x] For a side Root rotation only, retry a blocked opposite top-level family with existing pinned-circle packing. Keep mapped subtrees and the driving placement fixed and keep rollback if packing fails.
