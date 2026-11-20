cmake_minimum_required(VERSION 3.16)
project(HarnessSectionCore LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(HRN_BUILD_TESTS "Build tests" ON)
option(HRN_BUILD_DEMO "Build Stage 1 demo" OFF)
option(HRN_BUILD_VISUAL_REPORT "Build optional Stage 2 HTML report exporter" OFF)
option(HRN_BUILD_INTERACTIVE_DEBUG_TOOL "Build the local Stage 2 WebAssembly debugger" OFF)
option(HRN_ENABLE_PRODUCT_ADAPTER "Require product integration paths" OFF)
option(HRN_ENABLE_ASAN "Enable address sanitizer" OFF)
option(HRN_WARNINGS_AS_ERRORS "Treat project warnings as errors" OFF)
if(HRN_BUILD_TESTS OR HRN_BUILD_DEMO OR HRN_BUILD_VISUAL_REPORT)
  enable_testing()
endif()
set(HRN_MAX_STAGE 1 CACHE STRING "Maximum enabled implementation stage")
if(NOT HRN_MAX_STAGE MATCHES "^[123]$")
  message(FATAL_ERROR "HRN_MAX_STAGE must be 1, 2, or 3")
endif()
if(HRN_BUILD_VISUAL_REPORT AND HRN_MAX_STAGE STREQUAL "1")
  message(FATAL_ERROR "HRN_BUILD_VISUAL_REPORT requires HRN_MAX_STAGE=2 or 3")
endif()
if(HRN_BUILD_INTERACTIVE_DEBUG_TOOL)
  if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "HRN_BUILD_INTERACTIVE_DEBUG_TOOL requires Emscripten; use emcmake to configure")
  endif()
  if(HRN_MAX_STAGE STREQUAL "1")
    message(FATAL_ERROR "HRN_BUILD_INTERACTIVE_DEBUG_TOOL requires HRN_MAX_STAGE=2 or 3")
  endif()
endif()
include(cmake/HrnProductIntegration.cmake)
if(HRN_ENABLE_ASAN)
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address)
endif()
file(GLOB HRN_CORE_SOURCES CONFIGURE_DEPENDS src/section/*.cpp)
if(HRN_MAX_STAGE STREQUAL "1")
  foreach(stage2_source IN ITEMS
      HrnCircularGeometryService.cpp HrnCircularHierarchySolver.cpp
      HrnCircularRuleBasedLayoutSolver.cpp HrnCoupledCircularSolver.cpp
      HrnMinimumEnclosingCircle.cpp HrnPositionPointTransaction.cpp
      HrnSectionDiagnostics.cpp)
    list(REMOVE_ITEM HRN_CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/section/${stage2_source}")
  endforeach()
endif()
add_library(hrn_section_core ${HRN_CORE_SOURCES})
target_include_directories(hrn_section_core PUBLIC include)
target_compile_options(hrn_section_core PRIVATE -Wall -Wextra -Wpedantic)
if(HRN_WARNINGS_AS_ERRORS)
  target_compile_options(hrn_section_core PRIVATE -Werror)
endif()
set(HRN_BUILD_INTERACTIVE_TEST OFF)
if(HRN_BUILD_TESTS AND NOT EMSCRIPTEN AND NOT HRN_MAX_STAGE STREQUAL "1")
  set(HRN_BUILD_INTERACTIVE_TEST ON)
endif()
if(HRN_BUILD_VISUAL_REPORT OR HRN_BUILD_INTERACTIVE_DEBUG_TOOL OR HRN_BUILD_INTERACTIVE_TEST)
  add_library(hrn_visual_report demo/HrnSectionVisualReport.cpp)
  target_link_libraries(hrn_visual_report PUBLIC hrn_section_core)
  target_include_directories(hrn_visual_report PUBLIC demo)
  target_compile_options(hrn_visual_report PRIVATE -Wall -Wextra -Wpedantic)
  if(HRN_WARNINGS_AS_ERRORS)
    target_compile_options(hrn_visual_report PRIVATE -Werror)
  endif()
endif()
if(HRN_BUILD_INTERACTIVE_DEBUG_TOOL OR HRN_BUILD_INTERACTIVE_TEST)
  add_library(hrn_interactive_case STATIC demo/interactive/HrnInteractiveCase.cpp)
  target_link_libraries(hrn_interactive_case PUBLIC hrn_visual_report)
  target_include_directories(hrn_interactive_case PUBLIC demo/interactive)
  target_compile_options(hrn_interactive_case PRIVATE -Wall -Wextra -Wpedantic)
  if(HRN_WARNINGS_AS_ERRORS)
    target_compile_options(hrn_interactive_case PRIVATE -Werror)
  endif()
endif()
if(HRN_BUILD_INTERACTIVE_DEBUG_TOOL)
  set(hrn_interactive_link_dir "${CMAKE_CURRENT_BINARY_DIR}/interactive-link")
  add_executable(hrn_interactive_solver_linked demo/interactive/HrnInteractiveBindings.cpp)
  target_link_libraries(hrn_interactive_solver_linked PRIVATE hrn_interactive_case)
  target_link_options(hrn_interactive_solver_linked PRIVATE -lembind --no-entry
    -sMODULARIZE=1 -sEXPORT_ES6=1 -sENVIRONMENT=web -sALLOW_MEMORY_GROWTH=1)
  # Current Embind headers require C++17; the solver/core targets remain C++14.
  set_target_properties(hrn_interactive_solver_linked PROPERTIES
    CXX_STANDARD 17 SUFFIX ".js" OUTPUT_NAME hrn_interactive_solver
    RUNTIME_OUTPUT_DIRECTORY "${hrn_interactive_link_dir}")

  get_property(hrn_interactive_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  # CMake 3.20 added configuration expressions to BYPRODUCTS.
  if(NOT hrn_interactive_multi_config OR CMAKE_VERSION VERSION_GREATER_EQUAL "3.20")
    set(hrn_interactive_wasm_dir "${hrn_interactive_link_dir}")
    if(hrn_interactive_multi_config)
      string(APPEND hrn_interactive_wasm_dir "/$<CONFIG>")
    endif()
    # Emscripten emits the WASM alongside CMake's primary JS output.
    add_custom_command(TARGET hrn_interactive_solver_linked POST_BUILD
      BYPRODUCTS "${hrn_interactive_wasm_dir}/hrn_interactive_solver.wasm"
      COMMAND "${CMAKE_COMMAND}" -E true
      VERBATIM)
  endif()

  if(CMAKE_GENERATOR MATCHES "Makefiles")
    # Make cleans byproducts but only checks the primary output before linking.
    add_custom_target(hrn_interactive_check_link_outputs
      COMMAND "${CMAKE_COMMAND}" "-DLINKED_DIR=${hrn_interactive_link_dir}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/HrnCheckInteractiveLink.cmake"
      VERBATIM)
    add_dependencies(hrn_interactive_solver_linked hrn_interactive_check_link_outputs)
  endif()

  # Script mode tracks UI edits even when the solver did not need relinking.
  add_custom_target(hrn_interactive_solver ALL
    COMMAND "${CMAKE_COMMAND}"
      "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/demo/interactive"
      "-DLINKED_DIR=$<TARGET_FILE_DIR:hrn_interactive_solver_linked>"
      "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/interactive"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/HrnPackageInteractive.cmake"
    VERBATIM)
  add_dependencies(hrn_interactive_solver hrn_interactive_solver_linked)
endif()
if(HRN_BUILD_TESTS)
  if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
  endif()
  include(FetchContent)
  FetchContent_Declare(googletest
    URL https://codeload.github.com/google/googletest/tar.gz/refs/tags/v1.16.0
    URL_HASH SHA256=78c676fc63881529bf97bf9d45948d905a66833fbfa5318ea2cd7478cb98f399)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  include(GoogleTest)
  file(GLOB HRN_SUPPORT_SOURCES CONFIGURE_DEPENDS tests/support/*.cpp)
  if(HRN_SUPPORT_SOURCES)
    add_library(hrn_test_support STATIC ${HRN_SUPPORT_SOURCES})
    target_link_libraries(hrn_test_support PUBLIC hrn_section_core)
    target_include_directories(hrn_test_support PUBLIC tests tests/support)
    if(HRN_WARNINGS_AS_ERRORS)
      target_compile_options(hrn_test_support PRIVATE -Werror)
    endif()
  else()
    add_library(hrn_test_support INTERFACE)
    target_link_libraries(hrn_test_support INTERFACE hrn_section_core)
    target_include_directories(hrn_test_support INTERFACE tests tests/support)
  endif()
  file(GLOB HRN_TEST_SOURCES CONFIGURE_DEPENDS tests/unit/*_test.cpp tests/acceptance/*_test.cpp)
  if(HRN_MAX_STAGE STREQUAL "1")
    foreach(stage2_test IN ITEMS
        hrn_circular_geometry_test hrn_circular_hierarchy_solver_test
        hrn_circular_rule_layout_test hrn_circular_transaction_test
        hrn_section_diagnostics_test stage2_acceptance_test
        hrn_section_visual_report_test stage2_visual_report_acceptance_test)
      list(REMOVE_ITEM HRN_TEST_SOURCES
           "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/${stage2_test}.cpp"
           "${CMAKE_CURRENT_SOURCE_DIR}/tests/acceptance/${stage2_test}.cpp")
    endforeach()
  endif()
  if(NOT HRN_BUILD_VISUAL_REPORT)
    list(REMOVE_ITEM HRN_TEST_SOURCES
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/hrn_section_visual_report_test.cpp")
  endif()
  if(NOT HRN_BUILD_INTERACTIVE_TEST)
    list(REMOVE_ITEM HRN_TEST_SOURCES
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/hrn_interactive_case_test.cpp")
  endif()
  foreach(test_source IN LISTS HRN_TEST_SOURCES)
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE hrn_test_support GTest::gtest_main)
    if(test_name STREQUAL "hrn_section_visual_report_test" OR
       test_name STREQUAL "stage2_visual_report_acceptance_test")
      target_link_libraries(${test_name} PRIVATE hrn_visual_report)
    endif()
    if(test_name STREQUAL "hrn_interactive_case_test")
      target_link_libraries(${test_name} PRIVATE hrn_interactive_case)
    endif()
    if(HRN_WARNINGS_AS_ERRORS)
      target_compile_options(${test_name} PRIVATE -Werror)
    endif()
    gtest_discover_tests(${test_name} TEST_PREFIX "${test_name}.")
  endforeach()
endif()
if(HRN_BUILD_DEMO AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/demo/stage1_case_builder_demo.cpp)
  file(GLOB HRN_SUPPORT_SOURCES CONFIGURE_DEPENDS tests/support/*.cpp)
  add_executable(stage1_case_builder_demo demo/stage1_case_builder_demo.cpp ${HRN_SUPPORT_SOURCES})
  target_link_libraries(stage1_case_builder_demo PRIVATE hrn_section_core)
  target_include_directories(stage1_case_builder_demo PRIVATE tests/support)
  add_test(NAME stage1_case_builder_demo COMMAND stage1_case_builder_demo)
endif()
if(HRN_BUILD_VISUAL_REPORT)
  add_executable(stage2_visual_report_demo demo/stage2_visual_report_demo.cpp
      tests/support/HrnOwnedSegmentGraph.cpp tests/support/HrnSectionCaseBuilder.cpp)
  target_link_libraries(stage2_visual_report_demo PRIVATE hrn_visual_report)
  target_include_directories(stage2_visual_report_demo PRIVATE tests/support)
  if(HRN_WARNINGS_AS_ERRORS)
    target_compile_options(stage2_visual_report_demo PRIVATE -Werror)
  endif()
  if(HRN_BUILD_TESTS)
    add_test(NAME stage2_visual_report_acceptance
      COMMAND ${CMAKE_COMMAND}
        -DDEMO=$<TARGET_FILE:stage2_visual_report_demo>
        -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/visual-report-acceptance
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/acceptance/verify_stage2_visual_report.cmake)
  endif()
endif()
