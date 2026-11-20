# Independent circular rotation matrix

Open any fixture below with **导入 JSON** in the interactive debugger. Begin each manual check by importing the fixture again, then add the chosen delta to one rotation field. A rotation field may start at a nonzero angle. To try the derived case, import **Deep ordinary** and remove its only correspondence before editing the rotation.

The WASM smoke test solves each baseline with `solveCase`. It then calls `solveRotation` for both side Roots and every section at deltas of +45°, −90°, +180°, and +360°. Each edit starts from the original draft and its solved baseline.

| Fixture | Layout and relations | Sections | Rotation edits |
| --- | --- | ---: | ---: |
| [N_1](cases/N_1.json) | Positive siblings share one negative center | 3 | 20 |
| [1_N](cases/1_N.json) | Negative siblings share one positive center | 3 | 20 |
| [N_N](cases/N_N_same_position.json) | Transitive many-to-many class across both sides | 4 | 24 |
| [N_1 mixed radii](cases/N_1_mixed_radii.json) | Unequal same-position siblings beside an ordinary correspondence | 4 | 24 |
| [Partial nesting](cases/partial_nested.json) | Three cross-side links at different depths with auto-resizing compounds | 8 | 40 |
| [Same-side rotation](cases/same_side_rot.json) | Nonzero Root angles and a free top-level sibling on each side of a linked pair | 4 | 24 |
| [Singleton follows](cases/stage2-singleton-follows.json) | A singleton follows a mapped member in a crowded side | 4 | 24 |
| [Deep ordinary](cases/rotation-deep-ordinary.json) | Four section levels on the positive side and two on the negative side, with an ordinary leaf correspondence | 12 | 56 |
| [Nested transitive](cases/rotation-nested-transitive.json) | One transitive class of four nested members, with unequal radii and a spaced free sibling on each side | 10 | 48 |
| [Mixed constraints](cases/rotation-mixed-constraints.json) | Fixed and auto-resizing Roots and compounds, with positive spacing at both levels and no relations | 8 | 40 |
| [Empty side](cases/rotation-empty-side.json) | Active positive Root with a compound and an empty negative side | 3 | 20 |
| Deep ordinary without relations | Derived from **Deep ordinary** by clearing its relation list | 12 | 56 |
| **Total** | **12 baselines, 24 Roots, 75 sections** | **75** | **396** |

Each result must solve, preserve fixed radii, and retain incidental section input angles. Same-position members must share a center. Each active container must preserve its spacing at the boundary and between siblings. Siblings in the same same-position class may overlap. A side Root boundary applies when the side has at least two top-level sections.

Every reported global fallback must rotate all prior centers and absolute angles by the requested delta, within the fixture’s modeling tolerance, without changing any solved radius. In **Partial nesting**, changing `p-3` from 0° to 180° must use that path. Rotating `p-branch` by −90° in either **Deep ordinary** variant must move its children around the branch center and leave every other center in place. An edit to the empty Root must solve locally and preserve every center. A full turn must preserve every center.

With Playwright and Chromium available, run the smoke suite against the existing WASM build from the repository root.

```bash
python3 tests/browser/interactive_debugger_smoke_test.py --directory build-wasm/interactive
```
