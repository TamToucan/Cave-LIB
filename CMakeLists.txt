cmake_minimum_required(VERSION 3.20)

if(DEFINED BUILD_GODOT)
    set(BUILD_GODOT_CAVE ${BUILD_GODOT})
else()
    set(BUILD_GODOT_CAVE ON)
endif()

if (NOT ALL_SUBS_BUILD_ON)
    project(Cave-Lib)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/_deps)
    include(FetchContent)

    if(BUILD_GODOT_CAVE)
        FetchContent_Declare(
            GDExtension
            GIT_REPOSITORY https://github.com/godotengine/godot-cpp.git
            GIT_TAG godot-4.3-stable
        )
    endif()

    FetchContent_Declare(
        Libs
        GIT_REPOSITORY https://github.com/TamToucan/Libs.git
        GIT_TAG main
    )

    if(BUILD_GODOT_CAVE)
        FetchContent_MakeAvailable(GDExtension Libs)
    else()
        FetchContent_MakeAvailable(Libs)
    endif()
else()
    message(STATUS "GDCave-EXT is being built as part of Lott-EXT, skipping FetchContent.")
endif()

# If needed: link to GDExtension and Libs targets provided by the parent
# If you’re unsure what’s defined globally, declare those as interface targets
# or just assume they exist and let parent control that.

add_subdirectory(cave)

