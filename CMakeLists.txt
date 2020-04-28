cmake_minimum_required(VERSION 3.16)
set(TRTLE_VERSION 0.1.0)
project(trtle VERSION ${TRTLE_VERSION} LANGUAGES C)

add_library(trtle)
add_subdirectory(src)
target_include_directories(trtle PRIVATE "src" "include/trtle")
target_compile_definitions(trtle PRIVATE "TRTLE_INTERNAL")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "trtleConfigVersion.cmake"
    VERSION ${TRTLE_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install (DIRECTORY "include/trtle"
    DESTINATION "include"
)

install(TARGETS trtle
    EXPORT trtleTargets
    LIBRARY DESTINATION "lib"
    INCLUDES DESTINATION "include"
)

install(EXPORT trtleTargets
    FILE "trtleConfig.cmake"
    NAMESPACE trtle::
    DESTINATION "lib/cmake/trtle"
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/trtleConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/trtleConfigVersion.cmake"
    DESTINATION "lib/cmake/trtle"
)
