cmake_minimum_required(VERSION 3.16)

# An older index may still be in use. Keep its immutable assets and publish the
# new index only after every file it references exists.
file(MAKE_DIRECTORY "${OUTPUT_DIR}")

function(publish staged destination expected_hash)
  if(EXISTS "${destination}")
    file(SHA256 "${destination}" current_hash)
    if(expected_hash STREQUAL current_hash)
      file(REMOVE "${staged}")
      return()
    endif()
  endif()
  file(RENAME "${staged}" "${destination}")
endfunction()

function(publish_asset stem extension asset_name)
  set(staged "${OUTPUT_DIR}/${stem}.${extension}.tmp")
  file(SHA256 "${staged}" content_hash)
  set(name "${stem}.${content_hash}.${extension}")
  publish("${staged}" "${OUTPUT_DIR}/${name}" "${content_hash}")
  set("${asset_name}" "${name}" PARENT_SCOPE)
endfunction()

# Hash the staged copies, so even edits during the build cannot publish
# different contents under a fingerprint that browsers already have cached.
configure_file("${SOURCE_DIR}/app.js" "${OUTPUT_DIR}/app.js.tmp" COPYONLY)
configure_file("${SOURCE_DIR}/app.css" "${OUTPUT_DIR}/app.css.tmp" COPYONLY)
configure_file("${LINKED_DIR}/hrn_interactive_solver.wasm"
  "${OUTPUT_DIR}/hrn_interactive_solver.wasm.tmp" COPYONLY)
publish_asset(app js HRN_APP_JS)
publish_asset(app css HRN_APP_CSS)
publish_asset(hrn_interactive_solver wasm wasm_name)

# The Emscripten module resolves this basename relative to its own URL. Hash
# the rewritten bytes so a WASM-only change also gives the loader a new URL.
file(READ "${LINKED_DIR}/hrn_interactive_solver.js" solver_js)
string(FIND "${solver_js}" "hrn_interactive_solver.wasm" wasm_reference)
if(wasm_reference EQUAL -1)
  message(FATAL_ERROR "Emscripten loader did not reference hrn_interactive_solver.wasm")
endif()
string(REPLACE "hrn_interactive_solver.wasm" "${wasm_name}" solver_js "${solver_js}")
file(WRITE "${OUTPUT_DIR}/hrn_interactive_solver.js.tmp" "${solver_js}")
publish_asset(hrn_interactive_solver js HRN_SOLVER_JS)
configure_file("${SOURCE_DIR}/index.html" "${OUTPUT_DIR}/index.html.tmp" @ONLY)
file(SHA256 "${OUTPUT_DIR}/index.html.tmp" index_hash)
publish("${OUTPUT_DIR}/index.html.tmp" "${OUTPUT_DIR}/index.html" "${index_hash}")
