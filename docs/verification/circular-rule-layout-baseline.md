# Circular rule layout baseline

Source: `docs/references/截面管理.md`, section 2.1, interpreted as a new deterministic rule on 2026-11-17. The user explicitly removed the historical `HrnGetSection` parity requirement. No product source or executable was available; these are **new implementation reference vectors**, not historical output.

Algorithm version 1: stable descending radius order; first center at `(0,0)`, second on `+X`; for each later circle, enumerate pair tangent intersections in stable pair order (upper solution first), then eight directions around each placed circle. Keep the first feasible candidate with the smallest origin-anchored enclosing radius. Spacing applies to sibling disks and the parent boundary. One iteration places one member; `candidate_count` counts evaluated positions. A final independent circle geometry check guards success.

Seven frozen cases are in `tests/fixtures/circular_rule_layout_cases.h`. They cover 1, 2, 3, and 6 equal circles; unsorted radii; spacing 0.5; and six varied radii. Expected centers, parent radii, candidate counts, and iteration counts are asserted within `1e-9` for floating values.

Constrained extension (2026-11-19): a request may supply `fixed_placements` and a
`parent_radius_limit`. Fixed circles retain their coordinates and form initial
obstacles; only free members consume placement iterations. The same tangent
candidate enumeration also considers the parent origin and all eight single-circle
directions when fixed circles are present. Candidates outside the fixed boundary
are filtered during search. Equal enclosing-radius scores prefer positions closer
to the parent origin; remaining ties preserve enumeration order. These additions
apply to constrained requests; all seven unconstrained vectors remain unchanged.

Compiler: GNU C++ 13.3.0, C++14. Source revision: this branch's Task 8 commit. Product integration path and product baseline commit remain unresolved in `product-integration-profile.md`.
