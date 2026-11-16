# Stage 1 gate: standalone core PASS

Date: 2026-11-16 (Asia/Shanghai)

- Compiler: GNU C++ 13.3.0 at `/usr/bin/c++`; generated compile flags include `-std=c++14 -Wall -Wextra -Wpedantic`.
- Configure/build: `cmake -S . -B build-stage1 -DHRN_BUILD_TESTS=ON -DHRN_MAX_STAGE=1 && cmake --build build-stage1 --parallel` — PASS.
- Tests: `ctest --test-dir build-stage1 --output-on-failure` — 6/6 PASS, 0 failed.
- Demo: `cmake -S . -B build-demo -DHRN_BUILD_DEMO=ON -DHRN_BUILD_TESTS=OFF && cmake --build build-demo --target stage1_case_builder_demo --parallel && ctest --test-dir build-demo -R stage1_case_builder_demo --output-on-failure` — 1/1 PASS.
- Scope check: `rg -n 'HrnIntelligentLayout|std::optional|std::variant|std::make_unique' include src tests` — no matches.
- Product adapter gate: configuring `HRN_ENABLE_PRODUCT_ADAPTER=ON` without a product checkout fails at configure time as required. See [product integration profile](product-integration-profile.md); product integration is unresolved.

The acceptance fixture covers five Segment levels, six positive and three negative members, one-to-one, one-to-many and many-to-one relations, two explicit same-position edges, distinct owned Section/Shape/Layout objects, a translated rotated rigid matrix, positive side Root auto-resize, and persisted Rule Based solve settings. The tests validate candidate input construction only; Stage 1 has no layout solver or solved nonempty Position Point.
