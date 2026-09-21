# Vita Phase 0 stub build.
#
# Links the REAL psxrecomp runtime (every PSXRECOMP_RUNTIME_SOURCES TU) with
# the REAL recompiled OpenBIOS backend and HAND-WRITTEN stubs replacing the
# game/overlay generated C (not available yet: no US disc). Proves the whole
# runtime compiles for ARMv7 vitasdk, links into a Vita ELF, and survives
# vita-elf-create / vita-make-fself / vita-pack-vpk. Driven by build-vita.sh
# --runtime-stub; the real full build uses the repo-root CMakeLists instead.
cmake_minimum_required(VERSION 3.20)
project(XenogearsRecompVitaStub C CXX)

get_filename_component(XG_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(PSXRECOMP_ROOT "${XG_REPO_ROOT}/psxrecomp" CACHE PATH
    "psxrecomp framework root (pinned submodule)")

set(PSX_SDL_BACKEND "SDL2" CACHE STRING "SDL backend" FORCE)
set(PSX_SDL3_FETCH OFF CACHE BOOL "" FORCE)
set(PSX_ZLIB_FETCH OFF CACHE BOOL "" FORCE)
set(PSX_RECOMP_UI OFF CACHE BOOL "" FORCE)
set(PSX_NETPLAY OFF CACHE BOOL "" FORCE)
set(PSX_REWIND OFF CACHE BOOL "" FORCE)
set(PSX_DEBUG_TOOLS OFF CACHE BOOL "" FORCE)
set(PSX_DEBUG_OVERLAY OFF CACHE BOOL "" FORCE)
set(PSX_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)

include("${PSXRECOMP_ROOT}/runtime/runtime.cmake")

if(PSX_SDL_LIBRARY_DIRS)
    link_directories(${PSX_SDL_LIBRARY_DIRS})
endif()

# All-zero identity digests: the stub links no game image, so the runtime's
# identity gates (psx_game_identity_runtime) read them as present-but-unused,
# exactly like any other non-native-render configure.
psxrecomp_add_runtime_target(psx-runtime
    GAME_GENERATED_FULL_C     "${CMAKE_CURRENT_SOURCE_DIR}/game_full_stub.c"
    GAME_GENERATED_DISPATCH_C "${CMAKE_CURRENT_SOURCE_DIR}/game_dispatch_stub.c"
    GAME_OVERLAY_STATIC_C     "${CMAKE_CURRENT_SOURCE_DIR}/overlay_static_stub.c"
    EXTRAS_SOURCES            "${CMAKE_CURRENT_SOURCE_DIR}/xg_render_stub.c"
    WINDOW_TITLE              "XenogearsRecomp"
    EXE_NAME                  "XenogearsRecomp"
    DEBUG_PORT                4370
    GAME_EXTRA_IDENTITY_SHA256 "0000000000000000000000000000000000000000000000000000000000000000"
    GAME_MANIFEST_DIGEST_SHA256 "0000000000000000000000000000000000000000000000000000000000000000"
    PRELOADED_MODS_DIR NONE
)
target_include_directories(psx-runtime PRIVATE "${XG_REPO_ROOT}/native_renderer/include")
