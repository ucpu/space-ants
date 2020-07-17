cmake_minimum_required(VERSION 3.10)
project(space-ants CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/externals/cage/cmake")
include(cage_all)
cage_default_init()
cage_assets_add_data_path("${CMAKE_CURRENT_SOURCE_DIR}/data")

add_subdirectory(externals/cage)

file(GLOB_RECURSE space-ants-sources "sources/*")
add_executable(space-ants ${space-ants-sources})
target_link_libraries(space-ants cage-engine)
cage_ide_category(space-ants space-ants)
cage_ide_sort_files(space-ants)
cage_ide_working_dir_in_place(space-ants)
