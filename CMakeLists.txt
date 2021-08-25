cmake_minimum_required(VERSION 3.20)
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
        GIT_TAG e3e1fbcf025cf83413815751f7c33500e1314d57
        DOWNLOAD_ONLY YES
)

if (imgui_ADDED)
    # imgui has no CMake support, so we create our own target

    FILE(GLOB imgui_sources ${imgui_SOURCE_DIR}/*.cpp)
    FILE(GLOB imgui_sources_cpp ${imgui_SOURCE_DIR}/misc/cpp/*.cpp)

    set(IMGUI_DIR ${imgui_SOURCE_DIR})
    set(IMGUI_INCLUDE_DIR ${imgui_SOURCE_DIR})
    set(IMGUI_SOURCES ${imgui_sources} ${imgui_sources_cpp} ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl.cpp)
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
)

if (md4c_ADDED)
    # imgui has no CMake support, so we create our own target

#    FILE(GLOB md4c_sources ${md4c_SOURCE_DIR}/*.cpp)
#    FILE(GLOB md4c_sources_cpp ${md4c_SOURCE_DIR}/misc/cpp/*.cpp)
    add_definitions(-DMD4C_USE_UTF8)
    include_directories(${md4c_SOURCE_DIR}/src)
#    set(MD4C_DIR ${md4c_SOURCE_DIR})
#    set(MD4C_INCLUDE_DIR ${md4c_SOURCE_DIR})
#    set(MD4C_SOURCES ${md4c_sources} ${md4c_sources_cpp})
endif()

add_executable(md_render
        imgui_md.cpp
        imgui_md.h
        main.cpp
        ${IMGUI_SOURCES})

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

target_include_directories(md_render SYSTEM PUBLIC ${SDL2_INCLUDE_DIRS}
                                           PUBLIC ${IMGUI_INCLUDE_DIR}
                                           PUBLIC ${md4c_SOURCE_DIR}/src)
target_link_libraries(md_render ${SDL2_LIBRARIES} md4c OpenGL::GL)

file(COPY fonts DESTINATION ${CMAKE_BINARY_DIR})
file(COPY images DESTINATION ${CMAKE_BINARY_DIR})