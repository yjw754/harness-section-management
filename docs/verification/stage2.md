# Stage 2 gate: standalone core PASS

Gate date: 2026-11-17 (Asia/Shanghai). Product integration remains unresolved; this gate covers the standalone C++14 core only.

## Scope and rule baseline

The user explicitly replaced the planned historical `HrnGetSection` output parity gate with a new deterministic implementation based on `docs/references/截面管理.md`. The seven frozen vectors, exact algorithm order, and provenance are in [circular-rule-layout-baseline.md](circular-rule-layout-baseline.md). No historical source or executable was used.

Stage 2 contains only circular sections and Rule Based Layout. It computes child layouts bottom-up, uses a deterministic minimum enclosing circle to recenter each nested group and update its parent radius, then accumulates rotations and centers top-down into three-dimensional Locations. Explicit same-position edges form connected shared-center groups. The coupled solver performs bounded deterministic adjustments and an independent final geometry check. A failed fixed boundary, unresolved candidate search, or hard constraint leaves the formal Position Point unchanged.

## Verification

Host compiler: GNU C++ 13.3.0 (`/usr/bin/c++`), CMake 3.16.8, GoogleTest 1.16.0. Project compile flags include `-std=c++14 -Wall -Wextra -Wpedantic`; the gate build adds `-Werror`.

```sh
cmake -S . -B build-stage1-regression -DHRN_BUILD_TESTS=ON -DHRN_MAX_STAGE=1
cmake --build build-stage1-regression --parallel
ctest --test-dir build-stage1-regression --output-on-failure

cmake -S . -B build-stage2 -DHRN_BUILD_TESTS=ON -DHRN_MAX_STAGE=2 -DHRN_WARNINGS_AS_ERRORS=ON
cmake --build build-stage2 --parallel
ctest --test-dir build-stage2 --output-on-failure

cmake -S . -B build-stage2-asan -DHRN_MAX_STAGE=2 -DHRN_ENABLE_ASAN=ON
cmake --build build-stage2-asan --parallel
ctest --test-dir build-stage2-asan --output-on-failure

rg -n 'HrnIntelligentLayout|HrnEllipticalSectionShape|HrnRectangularSectionShape' include src
git diff --check
```

Results after the ownership and coupled-budget review fixes: Stage 1 regression **8/8 PASS**; Stage 1+2 gate **36/36 PASS**; AddressSanitizer **36/36 PASS** with no reported errors. The scope search returned no matches; `git diff --check` reported no whitespace errors.

Acceptance covers 100 positive-side circular Sections in five Segment levels and 50 negative-side Sections with a different tree, deterministic repeat solves, independent parent containment and sibling clearance checks, and a single Section edit transaction. A second five-level mapped case checks structured failure for incompatible explicit same-position constraints. Unit tests cover 0/1/N roots, fixed and automatic radius, three circular size modes, rotations, same-position classes, every formal edit entrance, rollback, unknown algorithm versions, budget failures, and reference vectors.

## Standalone performance baseline

One non-ASan run on this host (`./build-stage2/stage2_acceptance_test --gtest_filter=Stage2Acceptance.HundredCirclesFiveLevelsBothSidesAndRepeatability`) reported first solve **18.373 ms**, single-Section edit re-solve **19.028 ms**, **24,672** candidate evaluations, and **149** member-placement iterations across both sides (150 Sections total). Times are measurements, not enforced thresholds; product hardware and source integration are unresolved. Algorithm version 1 and these vectors are the Stage 3 circular regression baseline.

The bounded coupled repair can return `NoSolutionFound` for a geometrically feasible arrangement outside its deterministic candidate space. That reason does not claim mathematical infeasibility and its adjustment advice is marked heuristic.
