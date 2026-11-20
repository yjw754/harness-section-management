# Interactive Debugger Asset Fingerprints Implementation Plan

> **For agentic workers:** Apply `superpowers:test-driven-development` and verify against a regular browser reload. Leave this branch unmerged for user review.

**Goal:** A rebuilt debugger opens at the same local URL and loads the matching new JS and WASM on an ordinary refresh.

**Architecture and design:** Keep the user-selected content-fingerprinting approach, not a new port per build or disabling the entire cache. Emscripten links into a private staging directory. CMake publishes a fingerprinted WASM and Emscripten loader together, as well as versioned app JS/CSS; publish the stable HTML after its assets are available. Retain prior fingerprinted outputs so an older page can still load its own matching files. The bundled loopback server always fetches the stable entry point and allows successful fingerprinted assets to be cached.

**Tech Stack:** CMake 3.16, Emscripten, Python standard library and Playwright for verification. No solver or JSON schema changes.

**Spec:** User-selected design on 2026-11-20; debugger startup in `demo/interactive/README.md`.

## Task: Package, serve, and verify the versioned debugger

- [x] Add a real Chromium test to `tests/browser/interactive_debugger_smoke_test.py` for fingerprinted HTTP responses, unchanged rebuilds, and switching assets after an ordinary reload at the same URL. Confirm the old build fails the fingerprint check.
- [x] Change `CMakeLists.txt`, add `cmake/HrnPackageInteractive.cmake`, and template `demo/interactive/index.html`. Keep `hrn_interactive_solver` as the public build target; rewrite the generated loader's WASM basename before hashing the loader.
- [x] Add `demo/interactive/serve_debugger.py`, reuse its HTTP policy in browser verification, update tests that wrap the generated solver module, and document the stable entry point.
- [x] Rebuild WASM, run native Stage 1 and Stage 2 plus browser suites, and leave the stable loopback debugger running for review.

Verification: Stage 1 8/8, Stage 2 62/62, Chromium 29/29. Live-port inspection confirmed the entry is not stored, all four names match the SHA-256 of the actual responses, and `partial_nested.json` solves at zero rotation.
