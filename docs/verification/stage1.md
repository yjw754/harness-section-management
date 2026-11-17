# Stage 1 gate: standalone core PASS

Gate date: 2026-11-16; review fixes verified 2026-11-17 (Asia/Shanghai).

- Compiler: GNU C++ 13.3.0 at `/usr/bin/c++`; generated compile flags include `-std=c++14 -Wall -Wextra -Wpedantic`.
- Test framework: GoogleTest 1.16.0 (pinned SHA256 FetchContent archive), with individual cases registered through CTest.
- Compatibility check: CMake 3.16.8 configure/build and its CTest runner pass all 6 cases after the GoogleTest migration.
- clangd: `cmake -S . -B build-stage1 -DHRN_BUILD_TESTS=ON -DHRN_MAX_STAGE=1` exports `build-stage1/compile_commands.json`; `.clangd` points to it. After configuration, clangd checked all 18 source and test translation units with 0 errors.
- Configure/build: `cmake -S . -B build-stage1 -DHRN_BUILD_TESTS=ON -DHRN_MAX_STAGE=1 && cmake --build build-stage1 --parallel` — PASS.
- Tests: `ctest --test-dir build-stage1 --output-on-failure` — 6/6 PASS, 0 failed.
- Demo: `cmake -S . -B build-demo -DHRN_BUILD_DEMO=ON -DHRN_BUILD_TESTS=OFF && cmake --build build-demo --target stage1_case_builder_demo --parallel && ctest --test-dir build-demo -R stage1_case_builder_demo --output-on-failure` — 1/1 PASS.
- Scope check: `rg -n 'HrnIntelligentLayout|std::optional|std::variant|std::make_unique' include src tests` — no matches.
- Product adapter gate: configuring `HRN_ENABLE_PRODUCT_ADAPTER=ON` without a product checkout fails at configure time as required. See [product integration profile](product-integration-profile.md); product integration is unresolved.

The acceptance fixture covers five Segment levels, six positive and three negative members, one-to-one, one-to-many and many-to-one relations, two explicit same-position edges, distinct owned Section/Shape/Layout objects, a translated rotated rigid matrix, positive side Root auto-resize, and persisted Rule Based solve settings. The tests validate candidate input construction only; Stage 1 has no layout solver or solved nonempty Position Point.

Review follow-up: regression tests cover per-side Root derivation when a parent is absent from that side, reuse of a Segment across sides, failed Builder payload cleanup, Root validation against the final modeling tolerance, invalid enum values, aliased Root rejection, and angle normalization at the rounding boundary. The same six test executables pass with AddressSanitizer and leak detection enabled.
