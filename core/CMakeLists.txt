add_library(ims_core)

target_include_directories(ims_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/external
)

target_sources(ims_core PRIVATE
    src/catalogs.cpp
)

