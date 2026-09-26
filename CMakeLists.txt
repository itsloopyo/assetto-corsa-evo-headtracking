cmake_minimum_required(VERSION 3.20)
project(AssettoCorsaEvoHeadTracking VERSION 1.1.3 LANGUAGES C CXX)

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
    src/legacy_config/legacy_config.cpp
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

    # The frozen reader's value guards - headers only, nothing to link.
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

    # Exercises the real INI generator and the frozen loader, so it needs the
    # core's IniReader and the log sink both write their diagnostics to.
    ace_add_test(config_defaults src/config.cpp src/legacy_config/legacy_config.cpp)
    target_link_libraries(ace_config_defaults_tests PRIVATE cameraunlock)
    target_compile_definitions(ace_config_defaults_tests PRIVATE
        ACE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

    # The hotkey half of the frozen loader: which INI values reach a binding and
    # which leave the action on the key it already had.
    ace_add_test(config_hotkeys src/legacy_config/legacy_config.cpp)
    target_link_libraries(ace_config_hotkeys_tests PRIVATE cameraunlock)

    # The canonical config conversion's differential test: v1.1.3's reader and
    # startup code (the oracle, compiled from byte copies in
    # tests/config_differential/oracle) against the frozen reader in
    # src/legacy_config/.
    set(ACE_DIFFERENTIAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests/config_differential)
    add_executable(ace_config_differential
        ${ACE_DIFFERENTIAL_DIR}/differential_tests.cpp
        ${ACE_DIFFERENTIAL_DIR}/oracle/oracle_reader.cpp
        src/legacy_config/legacy_config.cpp
    )
    target_include_directories(ace_config_differential PRIVATE
        src
        ${ACE_DIFFERENTIAL_DIR}
        cameraunlock-core/cpp/include
    )
    target_compile_definitions(ace_config_differential PRIVATE
        ACE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(ace_config_differential PRIVATE cameraunlock)
    set_target_properties(ace_config_differential PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
    if(MSVC)
        target_compile_options(ace_config_differential PRIVATE /WX /EHsc /bigobj)
    endif()
    add_test(NAME config_differential COMMAND ace_config_differential)
    set_property(GLOBAL APPEND PROPERTY ACE_TEST_TARGETS ace_config_differential)

    # What the differential test compiles is pinned by hash, line endings aside:
    # the oracle's byte copies must stay v1.1.3's (git show v1.1.3:src/<file>),
    # the core sources both readers compile must stay the ones v1.1.3 was built
    # with (core f92be69), and the frozen reader is never edited.
    function(ace_require_hash path expected)
        file(READ "${path}" text)
        string(REPLACE "\r" "" text "${text}")
        string(SHA256 actual "${text}")
        if(NOT actual STREQUAL expected)
            message(FATAL_ERROR "${path} has changed (SHA-256 of its text ${actual}, pinned "
                                "${expected}). The config differential test compares against "
                                "exactly this file; see CMakeLists.txt.")
        endif()
    endfunction()
    set(ACE_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cameraunlock-core/cpp)
    foreach(pinned
            "${ACE_DIFFERENTIAL_DIR}/oracle/src/config.cpp|d00a1abcfc2dc1cc54ca9bb9f7f850f3fd5b41c4f2cc0ee49dfd8d219200d2f7"
            "${ACE_DIFFERENTIAL_DIR}/oracle/src/config.h|d6b7eaa7c4d4022d8946da693f854a567d0f88fde05c2bee45c5930ce9e70869"
            "${ACE_DIFFERENTIAL_DIR}/oracle/src/config_sanitize.h|65ffcad991781b4b58463eca4455fec55d885f4105807e708d56a0178d22bb93"
            "${ACE_DIFFERENTIAL_DIR}/oracle/src/logging.h|fabf37c1c6a37767a7ed25513ed93dbe8904ac5b13ebbcd2fc3d4b1b07c54947"
            "${ACE_CORE_DIR}/include/cameraunlock/config/ini_reader.h|a7ffb44210ff59672fa97e8e5feaa2cb3e81938fcc0334a384c68bc371b3857a"
            "${ACE_CORE_DIR}/src/config/ini_reader.cpp|e01515c2656aaf533bae4350dc45b702c3e3d4043935743dcc9bd5ea581fbe1c"
            "${ACE_CORE_DIR}/include/cameraunlock/protocol/port_utils.h|91bff564d5e279b66527ec5553afcf4d591812dab0e78f71a48ef412e4db7a44"
            "${ACE_CORE_DIR}/include/cameraunlock/math/smoothing_utils.h|fc2146f8c585e5f610c7234e302f59de4945679cfa28ff479ca47477ec073f22"
            "${ACE_CORE_DIR}/include/cameraunlock/math/angle_utils.h|d7a905270933e3cb0c4c361d29d3fd701655498cbcd1875ea79d180468bdbe6a"
            "${ACE_CORE_DIR}/include/cameraunlock/data/position_settings.h|b24dceb8e25475aebc5a468a5c7362a4a4e64204d183d1408525345f32f547f5"
            "${ACE_CORE_DIR}/include/cameraunlock/logging/file_log.h|43bdd2ef8554c78e5f440333463750c13b95110fe672b0b6e273244df9e7d169"
            "${ACE_CORE_DIR}/src/logging/file_log.cpp|73c53c2baa06bbfebe8211f62678aa2b60cb95f604743d3686951ba56b87ea47"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/legacy_config/legacy_config.h|f0efde9a3d6bf675fe3f7342796702cab8e334a5a6479aeeb412134b1b2540aa"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/legacy_config/legacy_config.cpp|7e94ab6fab9fd069c7714b279b7ba525afac2a8357f5959e0171f982455ab017"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/legacy_config/config_sanitize.h|c510e8c819b480a5060674d81ad447689220df8f10008c7b9e2b6b453a1ec70b")
        string(REPLACE "|" ";" pair "${pinned}")
        list(GET pair 0 pinned_path)
        list(GET pair 1 pinned_hash)
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${pinned_path}")
        ace_require_hash("${pinned_path}" "${pinned_hash}")
    endforeach()

    # Built from what ace_add_test registered, so a new test cannot be added to
    # ctest and left out of the target run-tests.ps1 builds.
    get_property(ace_test_targets GLOBAL PROPERTY ACE_TEST_TARGETS)
    add_custom_target(ace_tests DEPENDS ${ace_test_targets})
endif()
