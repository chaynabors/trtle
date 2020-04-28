cmake_minimum_required(VERSION 3.16)

set(TRTLE_VERSION_MAJOR 0)
set(TRTLE_VERSION_MINOR 1)
set(TRTLE_VERSION_PATCH 0)
set(TRTLE_VERSION "${TRTLE_VERSION_MAJOR}.${TRTLE_VERSION_MINOR}.${TRTLE_VERSION_PATCH}")

project(trtle LANGUAGES C)

set (TRTLE_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")
set (TRTLE_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include/trtle")

add_library(trtle)
add_subdirectory(${TRTLE_SOURCE_DIR})
target_include_directories(trtle PRIVATE "src")
target_include_directories(trtle PUBLIC
    $<BUILD_INTERFACE:${TRTLE_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)
set_property(TARGET trtle PROPERTY VERSION ${TRTLE_VERSION})
set_property(TARGET trtle PROPERTY SOVERSION ${TRTLE_VERSION_MAJOR})
target_compile_definitions(trtle PRIVATE "TRTLE_INTERNAL")

install(TARGETS trtle
    EXPORT trtleTargets
    LIBRARY DESTINATION "lib"
    INCLUDES DESTINATION "include"
)

install (DIRECTORY "include/trtle" DESTINATION "include")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/cmake/trtleVersion.cmake
    VERSION ${TRTLE_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(EXPORT trtleTargets
    FILE "trtleConfig.cmake"
    NAMESPACE trtle::
    DESTINATION "cmake"
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cmake/trtleConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/cmake/trtleConfigVersion.cmake"
    DESTINATION "cmake"
)
