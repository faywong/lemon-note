cmake_minimum_required(VERSION 3.20)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file")
project(imgui_md)

set(CMAKE_CXX_STANDARD 14)

#
#
# -----------------------------------------------------------------------------
# CPM
# -----------------------------------------------------------------------------

set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM.cmake")
set(CPM_DOWNLOAD_VERSION 0.32.2)

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION} AND CPM_VERSION STREQUAL CPM_DOWNLOAD_VERSION))
    message(STATUS "Downloading CPM.cmake")
    file(DOWNLOAD https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()

include(${CPM_DOWNLOAD_LOCATION})

#
#
# -----------------------------------------------------------------------------
# CPM: stb
# -----------------------------------------------------------------------------

CPMAddPackage(
    NAME stb
    GITHUB_REPOSITORY nothings/stb
    GIT_TAG 3a1174060a7dd4eb652d4e6854bc4cd98c159200
    DOWNLOAD_ONLY YES
)

if (stb_ADDED)
# stb has no CMake support, so we create our own target
include_directories(${stb_SOURCE_DIR})
endif()

#
#
# -----------------------------------------------------------------------------
# CPM: imgui
# -----------------------------------------------------------------------------

CPMAddPackage(
        NAME imgui
        GITHUB_REPOSITORY ocornut/imgui
        GIT_TAG v1.89.9
        DOWNLOAD_ONLY YES
)

if (imgui_ADDED)
    # imgui has no CMake support, so we create our own target

    FILE(GLOB imgui_sources ${imgui_SOURCE_DIR}/*.cpp)
    FILE(GLOB imgui_sources_cpp ${imgui_SOURCE_DIR}/misc/cpp/*.cpp)

    set(IMGUI_DIR ${imgui_SOURCE_DIR})
    set(IMGUI_INCLUDE_DIR ${imgui_SOURCE_DIR})
    include_directories(${imgui_SOURCE_DIR})
    set(IMGUI_SOURCES ${imgui_sources} ${imgui_sources_cpp} ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp 
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp 
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp)
endif()

#
#
# -----------------------------------------------------------------------------
# CPM: MD4C
# -----------------------------------------------------------------------------

CPMAddPackage(
        NAME md4c
        GITHUB_REPOSITORY mity/md4c
        GIT_TAG c3340b480e5232711858108be07460a9836c8ab5
        OPTIONS
#         "MD4C_USE_UTF8"
)

if (md4c_ADDED)
    include_directories(${md4c_SOURCE_DIR}/src)
endif()

#
#
# -----------------------------------------------------------------------------
# CPM: BalazsJako/ImGuiColorTextEdit
# -----------------------------------------------------------------------------

CPMAddPackage(
        NAME ImGuiColorTextEdit
        GITHUB_REPOSITORY BalazsJako/ImGuiColorTextEdit
        GIT_TAG 0a88824f7de8d0bd11d8419066caa7d3469395c4
)

if (ImGuiColorTextEdit_ADDED)
    include_directories(${ImGuiColorTextEdit_SOURCE_DIR})
endif()

#
#
# -----------------------------------------------------------------------------
# CPM: aiekick/ImGuiFileDialog
# -----------------------------------------------------------------------------

CPMAddPackage(
        NAME ImGuiFileDialog
        GITHUB_REPOSITORY aiekick/ImGuiFileDialog
        GIT_TAG v0.6.5
)

if (ImGuiFileDialog_ADDED)
    include_directories(${ImGuiFileDialog_SOURCE_DIR})
endif()

#
#
# -----------------------------------------------------------------------------
# CPM: pthom/hello_imgui
# -----------------------------------------------------------------------------
# CPMAddPackage(
#         NAME hello_imgui
#         GITHUB_REPOSITORY pthom/hello_imgui
#         GIT_TAG 0.8.0
#         OPTIONS
#         "HELLOIMGUI_USE_SDL_OPENGL3 ON"
# )

# if (hello_imgui_ADDED)
#     include_directories(${hello_imgui_SOURCE_DIR})
#     list(APPEND CMAKE_MODULE_PATH ${hello_imgui_SOURCE_DIR}/hello_imgui_cmake)
#     include(hello_imgui_add_app)
# endif()

#
#
# -----------------------------------------------------------------------------
# CPM: gulrak/filesystem
# -----------------------------------------------------------------------------
CPMAddPackage(
        NAME ghc_fs
        GITHUB_REPOSITORY gulrak/filesystem
        GIT_TAG v1.5.14
        OPTIONS
        "HELLOIMGUI_USE_SDL_OPENGL3 ON"
)

if (ghc_fs_ADDED)
    include_directories(${ghc_fs_SOURCE_DIR}/include)
endif()

add_executable(md_render
        imgui_md.cpp
        imgui_md.h
        ${ImGuiColorTextEdit_SOURCE_DIR}/TextEditor.cpp
        ${ImGuiFileDialog_SOURCE_DIR}/ImGuiFileDialog.cpp
        main.cpp
        ${IMGUI_SOURCES})

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS}
                    ${IMGUI_INCLUDE_DIR}
                    ${md4c_SOURCE_DIR}/src)

target_link_libraries(md_render PRIVATE ${SDL2_LIBRARIES} md4c OpenGL::GL)

file(COPY fonts DESTINATION ${CMAKE_BINARY_DIR})
file(COPY images DESTINATION ${CMAKE_BINARY_DIR})