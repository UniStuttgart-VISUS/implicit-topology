#
# MegaMol™ flowvis Plugin
# Copyright 2019, by MegaMol Team
# Alle Rechte vorbehalten. All rights reserved.
#
option(BUILD_FLOWVIS_PLUGIN "Option to build flowvis" ON)

if(BUILD_FLOWVIS_PLUGIN)
  if(NOT ENABLE_CUDA)
    message(FATAL_ERROR "The FlowVis plugin requires CUDA. Please enable it setting the option ENABLE_CUDA.")
  endif()

  project(flowvis)

  string(TOUPPER ${PROJECT_NAME} EXPORT_NAME)

  # Check option dependencies: First list item is always the current build option,
  # afterwards list the dependencies.
  set(DEP_LIST "${DEP_LIST};BUILD_FLOWVIS_PLUGIN BUILD_CORE" CACHE INTERNAL "")

  file(GLOB_RECURSE public_header_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "include/*.h")
  file(GLOB_RECURSE source_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "src/*.cpp")
  file(GLOB_RECURSE header_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "src/*.h")
  file(GLOB_RECURSE shader_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "shaders/*")
  file(GLOB_RECURSE thirdparty_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "3rdparty/*.cpp")

  if(UNIX)
    list(REMOVE_ITEM source_files "src/dllmain.cpp")
  endif()

  # Get TPF
  require_external(tpf)

  # Create CUDA library
  add_subdirectory(cuda)

  # Target definition
  add_library(${PROJECT_NAME} SHARED ${public_header_files} ${header_files} ${source_files} ${thirdparty_files})
  # Set target naming conventions for different build types
  set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".mmplg")
  target_compile_definitions(${PROJECT_NAME} PRIVATE ${EXPORT_NAME}_EXPORTS _ENABLE_EXTENDED_ALIGNED_STORAGE ${tpf_compile_definitions})
  target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> "include" "src" "3rdparty" PRIVATE ${CGAL_INCLUDE_DIRS} ${CGAL_3RD_PARTY_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} PRIVATE core mmstd_datatools mesh compositing_gl tpf flowvis_streamlines_cuda)

  # Installation rules for generated files
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION "include")
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders/ DESTINATION "share/shaders")
  if(WIN32)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "bin")
    install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION "lib")
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION bin OPTIONAL)
  else()
    install(TARGETS ${PROJECT_NAME} DESTINATION "lib" EXPORT ${PROJECT_NAME}-target)
  endif()

  # Grouping in Visual Studio
  set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER plugins)
  source_group("Public Header Files" FILES ${public_header_files})
  source_group("Header Files" FILES ${header_files})
  source_group("Source Files" FILES ${source_files})
  source_group("Shaders" FILES ${shader_files})
endif()
