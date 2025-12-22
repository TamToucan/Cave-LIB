cmake_minimum_required(VERSION 3.20)

# --- 1. STANDALONE SETUP (Only runs if not a submodule) ---
if(NOT ALL_SUBS_BUILD_ON)
    project(Cave-LIB)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    
    # Options
    if(DEFINED BUILD_GODOT)
        set(BUILD_GODOT_CAVE ${BUILD_GODOT})
    else()
        option(BUILD_GODOT "Build Godot support" OFF)
        set(BUILD_GODOT_CAVE ${BUILD_GODOT})
    endif()

    option(BUILD_CUTE "Build Cute Framework support" ON)

    # Dependencies
    include(FetchContent)
    set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/_deps)

    # Fetch Libs
    FetchContent_Declare(
        Libs
        GIT_REPOSITORY https://github.com/TamToucan/Libs.git
        GIT_TAG main
    )
    
    # Fetch Godot (Only if needed)
    if(BUILD_GODOT_CAVE)
        FetchContent_Declare(
            GDExtension
            GIT_REPOSITORY https://github.com/godotengine/godot-cpp.git
            GIT_TAG godot-4.3-stable
        )
        FetchContent_MakeAvailable(GDExtension Libs)
    else()
        FetchContent_MakeAvailable(Libs)
    endif()

    # Setup Cute (Standalone Mode)
    if(BUILD_CUTE)
        set(CUTE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/deps/cute")
        if(EXISTS "${CUTE_ROOT}/lib/libcute.a")
            set(CUTE_FOUND ON)
            add_library(cute STATIC IMPORTED GLOBAL)
            set_target_properties(cute PROPERTIES
                IMPORTED_LOCATION "${CUTE_ROOT}/lib/libcute.a"
                INTERFACE_INCLUDE_DIRECTORIES "${CUTE_ROOT}/include;${CUTE_ROOT}"
            )
            file(GLOB CUTE_DEPENDENCIES "${CUTE_ROOT}/lib/*.a")
            list(REMOVE_ITEM CUTE_DEPENDENCIES "${CUTE_ROOT}/lib/libcute.a")
            set_target_properties(cute PROPERTIES INTERFACE_LINK_LIBRARIES "${CUTE_DEPENDENCIES}")
        endif()
    endif()

else()
    message(STATUS "Cave-LIB is being built as a submodule.")
endif()


# --- 2. Build Core Library (The Fix) ---
# We point to the specific folder where we moved the core CMake logic
add_subdirectory(cave/src/core)
add_library(CaveLib::Cave ALIAS cave)


# --- 3. Build Godot Adapter (The Fix) ---
# We point to the specific folder where we moved the godot CMake logic
if(BUILD_GODOT OR BUILD_GODOT_CAVE)
    add_subdirectory(cave/src/godot)
endif()


# --- 4. Build Cute Adapter ---
# This logic lives here in the root (mirroring your Maze setup)
if(BUILD_CUTE AND CUTE_FOUND)
    # Ensure source file exists at 'cave/src/cute/cuteCave.cpp'
    add_library(CuteCave SHARED cave/src/cute/cuteCave.cpp)
    target_link_libraries(CuteCave PRIVATE CaveLib::Cave cute)
    target_include_directories(CuteCave PRIVATE cave/src/cute)
    
    # Windows: Copy DLL next to CuteCave so tests can run
    if(TARGET CaveLib::Cave)
         add_custom_command(TARGET CuteCave POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:CaveLib::Cave>
            $<TARGET_FILE_DIR:CuteCave>
        )
    endif()
endif()


# --- 5. Build Tests ---
option(BUILD_CAVE_TESTS "Build Cave library tests" ON)

if(BUILD_CAVE_TESTS)
    add_subdirectory(cave/test)
endif()

