cmake_minimum_required(VERSION 3.24)
project(imsimseq LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Where your personal SDK stash lives (I personally use a sibling Libraries folder)
set(LIBRARIES_DIR "${CMAKE_SOURCE_DIR}/../Libraries" CACHE PATH "Path to shared Libraries folder")
set(SDL3_SDK     "${LIBRARIES_DIR}/SDL3"            CACHE PATH "Path to SDL3 VC devel folder")
option(USE_LOCAL_SDL3 "Use local SDL3 SDK instead of find_package" ON)

add_subdirectory(core)

# SDL3: prefer your local VC SDK; otherwise defer to package managers
if(USE_LOCAL_SDL3 AND EXISTS "${SDL3_SDK}/include" AND EXISTS "${SDL3_SDK}/lib/x64/SDL3.lib")
  add_library(SDL3::SDL3 SHARED IMPORTED)
  set_target_properties(SDL3::SDL3 PROPERTIES
    IMPORTED_LOCATION             "${SDL3_SDK}/lib/x64/SDL3.dll"
    IMPORTED_IMPLIB               "${SDL3_SDK}/lib/x64/SDL3.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL3_SDK}/include"
  )
  set(USING_SDL3_LOCAL ON)
else()
  find_package(SDL3 CONFIG REQUIRED)
endif()

# GLAD (vendored)
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# ImGui (vendored, docking branch)
add_library(imgui STATIC
  external/imgui/imgui.cpp
  external/imgui/imgui_draw.cpp
  external/imgui/imgui_tables.cpp
  external/imgui/imgui_widgets.cpp
  external/imgui/backends/imgui_impl_sdl3.cpp
  external/imgui/backends/imgui_impl_opengl3.cpp
  # external/imgui/imgui_demo.cpp   # uncomment if you want the demo window
)
target_include_directories(imgui PUBLIC external/imgui external/imgui/backends)
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1)
# IMPORTANT: give imgui access to SDL headers via the imported target
target_link_libraries(imgui PRIVATE SDL3::SDL3)

add_executable(imsimseq src/main.cpp)

target_link_libraries(imsimseq PRIVATE
  ims_core
  SDL3::SDL3
  imgui
  glad
  opengl32
)

# Handy: drop SDL3.dll next to the exe if you're using the local SDK
if(USING_SDL3_LOCAL)
  add_custom_command(TARGET imsimseq POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL3_SDK}/lib/x64/SDL3.dll" "$<TARGET_FILE_DIR:imsimseq>/")
endif()
