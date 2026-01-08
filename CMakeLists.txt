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
        message(STATUS "Checking for Cute in: ${CUTE_ROOT}")
        
        if(EXISTS "${CUTE_ROOT}/lib/libcute.dll.a")
            message(STATUS "Found Cute DLL Import Library: ${CUTE_ROOT}/lib/libcute.dll.a")
            set(CUTE_FOUND ON)
            add_library(cute SHARED IMPORTED GLOBAL)
            set_target_properties(cute PROPERTIES
                IMPORTED_IMPLIB "${CUTE_ROOT}/lib/libcute.dll.a"
                IMPORTED_LOCATION "${CUTE_ROOT}/bin/libcute.dll"
                INTERFACE_INCLUDE_DIRECTORIES "${CUTE_ROOT}/include;${CUTE_ROOT}"
            )
            # Alias cute_lib to cute for consistency with submodule builds
            add_library(cute_lib ALIAS cute)
        elseif(EXISTS "${CUTE_ROOT}/lib/libcute.a")
            message(STATUS "Found Cute Static Library: ${CUTE_ROOT}/lib/libcute.a")
            message(WARNING "Cute static library found but missing dependencies (SDL). This might fail to link.")
            set(CUTE_FOUND ON)
            add_library(cute STATIC IMPORTED GLOBAL)
            set_target_properties(cute PROPERTIES
                IMPORTED_LOCATION "${CUTE_ROOT}/lib/libcute.a"
                INTERFACE_INCLUDE_DIRECTORIES "${CUTE_ROOT}/include;${CUTE_ROOT}"
                INTERFACE_COMPILE_DEFINITIONS "CF_STATIC"
            )
            file(GLOB CUTE_DEPENDENCIES "${CUTE_ROOT}/lib/*.a")
            list(REMOVE_ITEM CUTE_DEPENDENCIES "${CUTE_ROOT}/lib/libcute.a")
            set_target_properties(cute PROPERTIES INTERFACE_LINK_LIBRARIES "${CUTE_DEPENDENCIES}")
            
            # Alias cute_lib to cute for consistency with submodule builds
            add_library(cute_lib ALIAS cute)
        endif()
    endif()

else()
    message(STATUS "Cave-LIB is being built as a submodule.")
endif()


# --- 2. Build Core Library (The Fix) ---
# We point to the specific folder where we moved the core CMake logic
add_subdirectory(cave/src/core)
add_library(CaveLib::Cave ALIAS Cave)


# --- 3. Build Godot Adapter (The Fix) ---
# We point to the specific folder where we moved the godot CMake logic
if(BUILD_GODOT OR BUILD_GODOT_CAVE)
    add_subdirectory(cave/src/godot)
endif()


# --- 4. Build Cute Adapter ---
# This logic lives in a subdirectory for consistency
if(BUILD_CUTE AND CUTE_FOUND)
    add_subdirectory(cave/src/cute)
endif()


# --- 5. Build Tests ---
option(BUILD_CAVE_TESTS "Build Cave library tests" ON)

if(BUILD_CAVE_TESTS)
    add_subdirectory(cave/test)
endif()

