cmake_minimum_required(VERSION 3.20)
project(AssettoCorsaEvoHeadTracking VERSION 1.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(MSVC)
    add_compile_options(/permissive- /Zc:__cplusplus /MP /W4)
endif()

include(FetchContent)
# Pinned to the commit v1.3.3 points at, not the tag: a tag can be moved at
# any time, and this source is compiled into the binary users install.
FetchContent_Declare(
    minhook_src
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        9fbd087432700d73fc571118d6a9697a36443d88  # v1.3.3
)
FetchContent_MakeAvailable(minhook_src)
add_library(minhook STATIC
    ${minhook_src_SOURCE_DIR}/src/hook.c
    ${minhook_src_SOURCE_DIR}/src/buffer.c
    ${minhook_src_SOURCE_DIR}/src/trampoline.c
    ${minhook_src_SOURCE_DIR}/src/hde/hde64.c
    ${minhook_src_SOURCE_DIR}/src/hde/hde32.c
)
target_include_directories(minhook PUBLIC ${minhook_src_SOURCE_DIR}/include)
if(MSVC)
    target_compile_options(minhook PRIVATE /wd4267 /wd4244 /wd4127 /wd4131)
endif()

# DISCOVERY carries memory/rtti_vtable.cpp, which is how this mod resolves the
# camera vtables by RTTI class name instead of pinning their RVAs.
set(CAMERAUNLOCK_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CAMERAUNLOCK_BUILD_HOOKS ON CACHE BOOL "" FORCE)
set(CAMERAUNLOCK_BUILD_DISCOVERY ON CACHE BOOL "" FORCE)
add_subdirectory(cameraunlock-core/cpp ${CMAKE_BINARY_DIR}/cameraunlock-core)

set(MOD_SOURCES
    src/dllmain.cpp
    src/headtracking_mod.cpp
    src/config.cpp
    src/camera_hook.cpp
    src/camera_transform.cpp
    src/exe_paths.cpp
    src/sim_state.cpp
    src/builds/build_registry.cpp
    src/builds/steam_offsets.cpp
)

add_library(AssettoCorsaEvoHeadTracking SHARED ${MOD_SOURCES})

target_include_directories(AssettoCorsaEvoHeadTracking PRIVATE
    src
    cameraunlock-core/cpp/include
)

target_link_libraries(AssettoCorsaEvoHeadTracking PRIVATE
    cameraunlock
    cameraunlock_hooks
    cameraunlock_discovery
    minhook
    ws2_32
    psapi
)

# Output as .asi for Ultimate ASI Loader.
set_target_properties(AssettoCorsaEvoHeadTracking PROPERTIES
    SUFFIX ".asi"
    PREFIX ""
)

# Opt-in unit tests. OFF by default so the normal build and the release
# packager never produce a test binary next to the .asi. Enabled by
# scripts/run-tests.ps1 (`pixi run test`), which builds the ace_tests target.
option(ACE_BUILD_TESTS "Build AssettoCorsaEvoHeadTracking unit tests" OFF)
if(ACE_BUILD_TESTS)
    enable_testing()

    function(ace_add_test test_name)
        set(target "ace_${test_name}_tests")
        add_executable(${target} tests/${test_name}_tests.cpp ${ARGN})
        target_include_directories(${target} PRIVATE
            src
            cameraunlock-core/cpp/include
        )
        set_target_properties(${target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
        # /W4 and /permissive- are already on globally; tests additionally
        # treat warnings as errors.
        if(MSVC)
            target_compile_options(${target} PRIVATE /WX)
        endif()
        add_test(NAME ${test_name} COMMAND ${target})
        set_property(GLOBAL APPEND PROPERTY ACE_TEST_TARGETS ${target})
    endfunction()

    # Pure boundary logic - headers only, nothing to link.
    ace_add_test(config_sanitize)

    # Pure camera math, compiled straight from the production translation unit.
    ace_add_test(camera_transform src/camera_transform.cpp)

    # Where the log and the INI land. The split of a module path needs no game;
    # the running test EXE stands in for one.
    ace_add_test(exe_paths src/exe_paths.cpp)

    # The pause gate, compiled from the production translation unit. Reading
    # the page needs the game; the classification it feeds does not. Links the
    # core for the log sink sim_state.cpp reports mapping failures to.
    ace_add_test(sim_state src/sim_state.cpp)
    target_link_libraries(ace_sim_state_tests PRIVATE cameraunlock)

    # Exercises the real INI generator and loader, so it needs the core's
    # IniReader and the log sink config.cpp writes its diagnostics to.
    ace_add_test(config_defaults src/config.cpp)
    target_link_libraries(ace_config_defaults_tests PRIVATE cameraunlock)
    target_compile_definitions(ace_config_defaults_tests PRIVATE
        ACE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

    # The hotkey half of the same loader: which INI values reach a binding and
    # which leave the action on the key it already had.
    ace_add_test(config_hotkeys src/config.cpp)
    target_link_libraries(ace_config_hotkeys_tests PRIVATE cameraunlock)

    # Built from what ace_add_test registered, so a new test cannot be added to
    # ctest and left out of the target run-tests.ps1 builds.
    get_property(ace_test_targets GLOBAL PROPERTY ACE_TEST_TARGETS)
    add_custom_target(ace_tests DEPENDS ${ace_test_targets})
endif()
