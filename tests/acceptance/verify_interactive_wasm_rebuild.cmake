cmake_minimum_required(VERSION 3.16)

# Run with -DBUILD_DIR=<disposable emcmake build> -P <this script>.
if(NOT EXISTS "${BUILD_DIR}/CMakeCache.txt")
  message(FATAL_ERROR "BUILD_DIR must point to a configured interactive Emscripten build")
endif()

function(build_solver step)
  execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${BUILD_DIR}" --target hrn_interactive_solver
    RESULT_VARIABLE result)
  if(NOT result STREQUAL "0")
    message(FATAL_ERROR "${step} failed (build result ${result})")
  endif()
endfunction()

set(linked "${BUILD_DIR}/interactive-link/hrn_interactive_solver")
set(index "${BUILD_DIR}/interactive/index.html")
set(outputs "${linked}.js" "${linked}.wasm" "${index}")

build_solver("Initial build")
foreach(output IN LISTS outputs)
  if(NOT EXISTS "${output}")
    message(FATAL_ERROR "Initial build did not generate ${output}")
  endif()
endforeach()

foreach(extension IN ITEMS wasm js)
  set(missing "${linked}.${extension}")
  file(REMOVE "${missing}")
  build_solver("Rebuild after removing ${missing}")
  if(NOT EXISTS "${missing}")
    message(FATAL_ERROR "Rebuild did not restore ${missing}")
  endif()
endforeach()

# Wait past the timestamp resolution of CMake 3.16.
file(GLOB published "${BUILD_DIR}/interactive/*")
set(outputs "${linked}.js" "${linked}.wasm" ${published})
foreach(output IN LISTS outputs)
  file(TIMESTAMP "${output}" "timestamp_${output}" "%s")
endforeach()
execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 1)
build_solver("No-op build")
foreach(output IN LISTS outputs)
  file(TIMESTAMP "${output}" timestamp "%s")
  if(NOT timestamp STREQUAL "${timestamp_${output}}")
    message(FATAL_ERROR "No-op build changed ${output}")
  endif()
endforeach()
