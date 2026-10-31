cmake_minimum_required(VERSION 3.21)
project(Kleiotide VERSION 0.29.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
option(KLEIOTIDE_BUILD_GUI "Build the Qt Widgets application" ON)
add_library(kleiotide_core src/core/abif_export.cpp src/core/collapse.cpp src/core/distances.cpp src/core/translation.cpp src/core/genetic_codes.cpp src/core/site_summary.cpp src/core/composition.cpp src/core/trace_quality.cpp src/core/groups.cpp src/core/group_import.cpp src/core/motif_index.cpp src/core/row_selection.cpp src/core/trace_coordinates.cpp src/core/session.cpp src/core/abif.cpp src/core/chromatogram.cpp src/core/alignment.cpp src/core/fasta.cpp src/core/overview.cpp src/core/sorting.cpp src/core/mafft_alignment.cpp src/core/mafft_template.cpp src/core/statistics.cpp)
configure_file(src/core/version.hpp.in generated/core/version.hpp @ONLY)
target_include_directories(kleiotide_core PUBLIC src "${CMAKE_CURRENT_BINARY_DIR}/generated")
if(MSVC)
  target_compile_options(kleiotide_core PRIVATE /W4)
else()
  target_compile_options(kleiotide_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
include(CTest)
target_sources(kleiotide_core PRIVATE src/core/trace_linking.cpp src/core/merge.cpp src/core/identity_groups.cpp)
if(BUILD_TESTING)
  add_executable(kleiotide_trace_linking_tests tests/trace_linking_tests.cpp)
  target_link_libraries(kleiotide_trace_linking_tests PRIVATE kleiotide_core)
  add_test(NAME trace_linking COMMAND kleiotide_trace_linking_tests)
  add_executable(kleiotide_overview_bounds_tests tests/overview_bounds_tests.cpp)
  target_link_libraries(kleiotide_overview_bounds_tests PRIVATE kleiotide_core)
  add_test(NAME overview_bounds COMMAND kleiotide_overview_bounds_tests)
  add_executable(kleiotide_row_index_tests tests/row_index_tests.cpp)
  target_link_libraries(kleiotide_row_index_tests PRIVATE kleiotide_core)
  add_test(NAME row_index COMMAND kleiotide_row_index_tests)
  add_executable(kleiotide_identity_group_tests tests/identity_group_tests.cpp)
  target_link_libraries(kleiotide_identity_group_tests PRIVATE kleiotide_core)
  add_test(NAME identity_groups COMMAND kleiotide_identity_group_tests)
  add_executable(kleiotide_abif_recovery_tests tests/abif_recovery_tests.cpp)
  target_link_libraries(kleiotide_abif_recovery_tests PRIVATE kleiotide_core)
  add_test(NAME abif_recovery COMMAND kleiotide_abif_recovery_tests)
  add_executable(kleiotide_merge_tests tests/merge_tests.cpp)
  target_link_libraries(kleiotide_merge_tests PRIVATE kleiotide_core)
  add_test(NAME merge COMMAND kleiotide_merge_tests)
  add_executable(kleiotide_abif_export_tests tests/abif_export_tests.cpp)
  target_link_libraries(kleiotide_abif_export_tests PRIVATE kleiotide_core)
  add_test(NAME abif_export COMMAND kleiotide_abif_export_tests)
  add_executable(kleiotide_collapse_tests tests/collapse_tests.cpp)
  target_link_libraries(kleiotide_collapse_tests PRIVATE kleiotide_core)
  add_test(NAME collapse COMMAND kleiotide_collapse_tests)
  add_executable(kleiotide_protein_mafft_tests tests/protein_mafft_tests.cpp)
  target_link_libraries(kleiotide_protein_mafft_tests PRIVATE kleiotide_core)
  add_test(NAME protein_mafft COMMAND kleiotide_protein_mafft_tests)
  add_executable(kleiotide_distance_tests tests/distance_tests.cpp)
  target_link_libraries(kleiotide_distance_tests PRIVATE kleiotide_core)
  add_test(NAME distances COMMAND kleiotide_distance_tests)
  add_executable(kleiotide_translation_tests tests/translation_tests.cpp)
  target_link_libraries(kleiotide_translation_tests PRIVATE kleiotide_core)
  add_test(NAME translation COMMAND kleiotide_translation_tests)
  add_executable(kleiotide_site_tests tests/site_analysis_tests.cpp)
  target_link_libraries(kleiotide_site_tests PRIVATE kleiotide_core)
  add_test(NAME site_analysis COMMAND kleiotide_site_tests)
  add_executable(kleiotide_quality_tests tests/trace_quality_tests.cpp)
  target_link_libraries(kleiotide_quality_tests PRIVATE kleiotide_core)
  add_test(NAME trace_quality COMMAND kleiotide_quality_tests)
  add_executable(kleiotide_group_tests tests/group_tests.cpp)
  target_link_libraries(kleiotide_group_tests PRIVATE kleiotide_core)
  add_test(NAME groups COMMAND kleiotide_group_tests)
  add_executable(kleiotide_type_tests tests/type_tests.cpp)
  target_link_libraries(kleiotide_type_tests PRIVATE kleiotide_core)
  add_test(NAME sequence_types COMMAND kleiotide_type_tests)
  add_executable(kleiotide_selection_tests tests/selection_tests.cpp)
  target_link_libraries(kleiotide_selection_tests PRIVATE kleiotide_core)
  add_test(NAME selections COMMAND kleiotide_selection_tests)
  add_executable(kleiotide_core_tests tests/core_tests.cpp)
  target_link_libraries(kleiotide_core_tests PRIVATE kleiotide_core)
  add_test(NAME core COMMAND kleiotide_core_tests)
  add_executable(kleiotide_trace_tests tests/trace_tests.cpp)
  target_link_libraries(kleiotide_trace_tests PRIVATE kleiotide_core)
  add_test(NAME traces COMMAND kleiotide_trace_tests)
  add_executable(kleiotide_session_tests tests/session_tests.cpp)
  target_link_libraries(kleiotide_session_tests PRIVATE kleiotide_core)
  add_test(NAME sessions COMMAND kleiotide_session_tests)
  add_executable(kleiotide_coordinates_tests tests/trace_coordinates_tests.cpp)
  target_link_libraries(kleiotide_coordinates_tests PRIVATE kleiotide_core)
  add_test(NAME coordinates COMMAND kleiotide_coordinates_tests)
endif()
option(KLEIOTIDE_BUILD_DEVELOPER_TOOLS "Build probes and benchmarks" OFF)
if(KLEIOTIDE_BUILD_DEVELOPER_TOOLS)
  foreach(tool abif_export_probe identity_group_benchmark core_benchmark row_index_benchmark)
    add_executable(kleiotide_${tool} tools/${tool}.cpp)
    target_link_libraries(kleiotide_${tool} PRIVATE kleiotide_core)
  endforeach()
endif()
option(KLEIOTIDE_BUILD_VIEWPORT_CHECK "Build viewport validation tools" OFF)
option(KLEIOTIDE_BUILD_GUI_SMOKE_CHECK "Build historical GUI workflow tools" OFF)
option(KLEIOTIDE_BUILD_RELEASE_CHECKS "Build and register the release GUI regression suite" OFF)
if(KLEIOTIDE_BUILD_GUI)
  find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Concurrent Network)
  qt_standard_project_setup()
  # Qt Creator's Run environment and the build/CTest environment can differ.
  # Use the configured Qt DLL target so Release/Debug and custom kits agree.
  function(kleiotide_qt_test_runtime test_name)
    if(WIN32)
      if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.22)
        set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
      else()
        # ENVIRONMENT_MODIFICATION was added in 3.22; retain the 3.21 minimum.
        set(qt_test_path "$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}")
        string(REPLACE ";" "\\;" qt_test_path "${qt_test_path}")
        set_property(TEST "${test_name}" APPEND PROPERTY ENVIRONMENT "PATH=${qt_test_path}")
      endif()
    endif()
  endfunction()
  add_library(kleiotide_gui STATIC
    src/gui/dialog_policy.cpp
    src/gui/local_blast.cpp
    src/gui/spart.cpp
    src/gui/table_io.cpp
    third_party/miniz/miniz_tinfl.cpp
    src/gui/open_requests.cpp
    src/gui/alignment_view.cpp
    src/gui/residue_clipboard.cpp
    src/gui/recovery.cpp
    src/gui/app_settings.cpp
    src/gui/settings_dialog.cpp
    src/gui/main_window.cpp
    src/gui/alignment_overview.cpp
    src/gui/file_output.cpp
    src/gui/trace_link_dialog.cpp
    src/gui/blast_data.cpp
    src/gui/blast_dialog.cpp
    src/gui/group_import_dialog.cpp
    src/gui/identity_group_dialog.cpp
    src/gui/merge_dialog.cpp
    src/gui/abif_batch_export.cpp
    src/gui/abif_export_dialog.cpp
    src/gui/collapse_dialog.cpp
    src/gui/distance_dialog.cpp
    src/gui/translation_ui.cpp
    src/gui/site_highlighter.cpp
    src/gui/composition_dialog.cpp
    src/gui/quality_dialog.cpp
    src/gui/groups_dialog.cpp
    src/gui/clipboard.cpp
    src/gui/motif_search.cpp
    src/gui/motif_navigator.cpp
    src/gui/rename_rules.cpp
    src/gui/rename_dialog.cpp
    src/gui/sequence_selection.cpp
    src/gui/mafft_run.cpp
    src/gui/mafft_dialog.cpp
    src/gui/chromatogram_view.cpp
  )
  target_include_directories(kleiotide_gui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_compile_definitions(kleiotide_gui PRIVATE MINIZ_NO_STDIO MINIZ_NO_TIME MINIZ_NO_ARCHIVE_APIS MINIZ_NO_ZLIB_APIS)
  target_link_libraries(kleiotide_gui PUBLIC kleiotide_core Qt6::Widgets Qt6::Concurrent Qt6::Network)
  if(MSVC)
    target_compile_options(kleiotide_gui PRIVATE /W4)
  else()
    target_compile_options(kleiotide_gui PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  qt_add_executable(kleiotide src/gui/main.cpp)
  target_link_libraries(kleiotide PRIVATE kleiotide_gui)
  set_target_properties(kleiotide PROPERTIES WIN32_EXECUTABLE TRUE MACOSX_BUNDLE TRUE)
  install(TARGETS kleiotide RUNTIME DESTINATION . BUNDLE DESTINATION .)
  install(FILES tools/windows/Register-Kleiotide.ps1 DESTINATION tools)
  install(FILES third_party/miniz/LICENSE third_party/miniz/README.md DESTINATION third-party/miniz)
  install(FILES
    RELEASE_0.29.1_TESTING.md
    LOCAL_BLAST.md
    SPART_AND_TABLES.md
    BLAST_AND_TRACE_LINKING.md
    RELEASE_0.28.1_TESTING.md
    AB1_EXPORT.md
    AB1_IMPORT_RECOVERY.md
    BRANDING.md
    CHANGELOG.md
    COLLAPSE.md
    DEPENDENCIES.md
    DISPLAY_COLORS.md
    DISTANCES.md
    IDENTITY_GROUPS.md
    ICON_GUIDELINES.md
    LICENSE-STATUS.md
    LICENSE
    MAFFT_PROTEINS.md
    MERGING.md
    NEXT_STEPS.md
    IMPLEMENTATION_STATUS.md
    PROTEIN_TRANSLATION_PLAN.md
    README.md
    RELEASE_CHECKLIST.md
    RELEASE_PLAN_0.27.1_0.28.0.md
    RELEASE_0.27.1_TESTING.md
    RELEASE_0.27.2_TESTING.md
    ROADMAP_1.0.md
    SESSION_FORMAT.md
    SETTINGS_AND_RECOVERY.md
    SITE_ANALYSIS.md
    TRANSLATION.md
    VALIDATION.md
    WINDOWS_TESTING.md
    WINDOWS_ACCEPTANCE.md
    WINDOWS_INSTALLER.md
    ARM64_COMPATIBILITY.md
    TESTER_FEEDBACK.md
    DESTINATION .)
  if(BUILD_TESTING)
    foreach(test motif_search rename file_output preferences recovery blast_data exchange)
      add_executable(kleiotide_${test}_tests tests/${test}_tests.cpp)
      target_link_libraries(kleiotide_${test}_tests PRIVATE kleiotide_gui)
      add_test(NAME ${test} COMMAND kleiotide_${test}_tests)
      kleiotide_qt_test_runtime(${test})
    endforeach()
    set_property(TEST exchange file_output preferences recovery APPEND PROPERTY ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_executable(kleiotide_residue_style_tests tests/residue_style_tests.cpp)
    target_link_libraries(kleiotide_residue_style_tests PRIVATE kleiotide_gui)
    add_test(NAME residue_styles COMMAND kleiotide_residue_style_tests)
    kleiotide_qt_test_runtime(residue_styles)
  endif()
  function(kleiotide_workflow name)
    if(NOT TARGET kleiotide_${name})
      qt_add_executable(kleiotide_${name} tools/${name}.cpp)
      target_link_libraries(kleiotide_${name} PRIVATE kleiotide_gui)
    endif()
  endfunction()
  if(KLEIOTIDE_BUILD_VIEWPORT_CHECK)
    kleiotide_workflow(drag_history_check)
    kleiotide_workflow(viewport_check)
  endif()
  if(KLEIOTIDE_BUILD_GUI_SMOKE_CHECK)
    kleiotide_workflow(identity_group_check)
    kleiotide_workflow(merge_check)
    kleiotide_workflow(abif_export_check)
    kleiotide_workflow(collapse_check)
    kleiotide_workflow(distance_check)
    kleiotide_workflow(translation_controls_check)
    kleiotide_workflow(translation_check)
    kleiotide_workflow(trace_selection_check)
    kleiotide_workflow(stage0193_check)
    kleiotide_workflow(stage0192_check)
    kleiotide_workflow(stage0191_check)
    kleiotide_workflow(stage019_check)
    kleiotide_workflow(stage018_check)
    kleiotide_workflow(stage0172_check)
    kleiotide_workflow(stage017_check)
    kleiotide_workflow(stage016_check)
    kleiotide_workflow(stage0151_check)
    kleiotide_workflow(stage015_check)
    kleiotide_workflow(stage014_check)
    kleiotide_workflow(stage013_check)
    kleiotide_workflow(stage012_check)
    kleiotide_workflow(stage011_check)
    kleiotide_workflow(gui_smoke)
    kleiotide_workflow(fragment_check)
    kleiotide_workflow(overview_check)
    kleiotide_workflow(view_layout_check)
    kleiotide_workflow(stage04_check)
    kleiotide_workflow(protein_mafft_window_check)
    kleiotide_workflow(protein_mafft_check)
    kleiotide_workflow(mafft_fixture)
    kleiotide_workflow(blast_fixture)
    kleiotide_workflow(mafft_check)
    kleiotide_workflow(stage010_check)
    kleiotide_workflow(stage09_check)
    kleiotide_workflow(stage08_check)
    kleiotide_workflow(stage07_check)
    kleiotide_workflow(stage06_check)
  endif()
  if(KLEIOTIDE_BUILD_RELEASE_CHECKS)
    if(NOT BUILD_TESTING)
      message(FATAL_ERROR "Release checks require BUILD_TESTING=ON")
    endif()
    kleiotide_workflow(mafft_fixture)
    kleiotide_workflow(blast_fixture)
    set(release_workflows abif_import_check save_safety_check identity_group_check translation_check
        translation_controls_check merge_check abif_export_check collapse_check distance_check mafft_check stage0191_check settings_check release_fixes_check blast_trace_check local_blast_check exchange_gui_check auto_preview_check)
    foreach(workflow IN LISTS release_workflows)
      kleiotide_workflow(${workflow})
      set(workflow_test_name "gui.${workflow}")
      if(workflow STREQUAL "stage0191_check")
        set(workflow_test_name "gui.mode_controls")
      endif()
      file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qa/${workflow}")
      set(workflow_output "${CMAKE_CURRENT_BINARY_DIR}/qa/${workflow}")
      if(workflow STREQUAL "abif_export_check" OR workflow STREQUAL "collapse_check" OR workflow STREQUAL "stage0191_check")
        set(workflow_output "${workflow_output}/preview.png")
      endif()
      if(workflow STREQUAL "blast_trace_check")
        add_dependencies(kleiotide_blast_trace_check kleiotide_blast_fixture)
        add_test(NAME ${workflow_test_name} COMMAND kleiotide_${workflow} "${workflow_output}" $<TARGET_FILE:kleiotide_blast_fixture>)
      elseif(workflow STREQUAL "save_safety_check")
        add_dependencies(kleiotide_save_safety_check kleiotide_mafft_fixture)
        add_test(NAME ${workflow_test_name} COMMAND kleiotide_${workflow} "${workflow_output}"
            $<TARGET_FILE:kleiotide_mafft_fixture>)
      elseif(workflow STREQUAL "settings_check")
        add_dependencies(kleiotide_settings_check kleiotide_mafft_fixture)
        add_test(NAME ${workflow_test_name} COMMAND kleiotide_${workflow} "${workflow_output}" $<TARGET_FILE:kleiotide_mafft_fixture>)
      elseif(workflow STREQUAL "mafft_check")
        add_dependencies(kleiotide_mafft_check kleiotide_mafft_fixture)
        add_test(NAME ${workflow_test_name} COMMAND kleiotide_${workflow} $<TARGET_FILE:kleiotide_mafft_fixture>)
      else()
        add_test(NAME ${workflow_test_name} COMMAND kleiotide_${workflow} "${workflow_output}")
      endif()
      set_tests_properties(${workflow_test_name} PROPERTIES LABELS "gui;release"
          ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 180)
      kleiotide_qt_test_runtime(${workflow_test_name})
    endforeach()
    add_custom_target(release-check
      COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --no-tests=error -C $<CONFIG>
          --output-log "${CMAKE_CURRENT_BINARY_DIR}/release-tests-$<CONFIG>.log"
          --output-junit "${CMAKE_CURRENT_BINARY_DIR}/release-tests-$<CONFIG>.xml"
      DEPENDS kleiotide
      USES_TERMINAL)
    get_property(directory_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS directory_targets)
      if(target MATCHES "^kleiotide_.*_tests$")
        add_dependencies(release-check ${target})
      endif()
    endforeach()
    # Target names carry the application prefix; keep dependencies explicit.
    foreach(workflow IN LISTS release_workflows)
      add_dependencies(release-check kleiotide_${workflow})
    endforeach()
  endif()
endif()

if(KLEIOTIDE_BUILD_GUI)
  include(tools/windows/installer.cmake)
endif()
