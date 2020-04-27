add_subdirectory(core)

target_sources(trtle PRIVATE pch.h)
target_include_directories(trtle PRIVATE core)
target_precompile_headers(trtle PRIVATE pch.h)
