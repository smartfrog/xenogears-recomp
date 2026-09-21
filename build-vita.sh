#!/usr/bin/env bash
# build-vita.sh — PS Vita cross-build driver for XenogearsRecomp.
#
# Usage:
#   ./build-vita.sh                # full build (requires generated/ game C + disc)
#   ./build-vita.sh --runtime-stub # Phase 0: real runtime + real OpenBIOS +
#                                  # hand-written stubs for the game/overlay
#                                  # generated code, packaged as a .vpk
#
# Environment:
#   VITASDK     VitaSDK root (default: $HOME/vitasdk)
#   BUILD_JOBS  parallel jobs (default: min(nproc, 8); the giant links OOM)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
MODE="full"
for arg in "$@"; do
    case "$arg" in
        --runtime-stub) MODE="runtime-stub" ;;
        *) echo "!!> ERROR: unknown argument: $arg"; exit 1 ;;
    esac
done

export VITASDK="${VITASDK:-$HOME/vitasdk}"
if [[ ! -d "$VITASDK" ]]; then
    echo "!!> ERROR: VITASDK not found: $VITASDK"
    exit 1
fi
export PATH="$VITASDK/bin:$HOME/.local/bin:$PATH"
for tool in arm-vita-eabi-gcc arm-vita-eabi-g++ vita-elf-create \
        vita-make-fself vita-mksfoex vita-pack-vpk; do
    command -v "$tool" &>/dev/null || {
        echo "!!> ERROR: required tool not on PATH: $tool (VITASDK=$VITASDK)"
        exit 1
    }
done

PARALLEL="${BUILD_JOBS:-$(nproc)}"
(( PARALLEL > 8 )) && PARALLEL=8

CMAKE_ARGS=(
    -G Ninja
    -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake"
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    -DPSX_SDL_BACKEND=SDL2
    -DPSX_RECOMP_UI=OFF
    -DPSX_NETPLAY=OFF
    -DPSX_REWIND=OFF
    -DPSX_DEBUG_TOOLS=OFF
    -DPSX_DEBUG_OVERLAY=OFF
    -DBUILD_TESTING=OFF
    -DXG_RENDER_NATIVE=OFF
    -DXG_RENDER_VALIDATE_OVERLAYS=OFF
)

if [[ "$MODE" == "runtime-stub" ]]; then
    BUILD_DIR="$ROOT/build-vita-stub"
    echo "==> [stub] Configuring Vita runtime-stub build in $BUILD_DIR..."
    cmake -S "$ROOT/vita/stub" -B "$BUILD_DIR" "${CMAKE_ARGS[@]}"
    echo "==> [stub] Building (real runtime + real OpenBIOS + stub game C)..."
    cmake --build "$BUILD_DIR" --parallel "$PARALLEL"
    ELF="$BUILD_DIR/XenogearsRecomp"
else
    BUILD_DIR="$ROOT/build-vita"
    if ! ls "$ROOT"/generated/slus_006.64_full_*.c &>/dev/null; then
        echo "!!> ERROR: generated/slus_006.64_full_*.c missing."
        echo "    Run the recompiler first (needs the legally owned US disc):"
        echo "      psxrecomp/recompiler/build/psxrecomp-game --config game.toml"
        exit 1
    fi
    DISC_IMAGE="${XG_DISC:-}"
    if [[ -z "$DISC_IMAGE" ]]; then
        for CANDIDATE in "$ROOT/game/disc1.cue" "$ROOT/game/disc1.bin" \
                "$ROOT/game/disc1.iso"; do
            [[ -f "$CANDIDATE" ]] && DISC_IMAGE="$CANDIDATE" && break
        done
    fi
    if [[ -z "$DISC_IMAGE" || ! -f "$DISC_IMAGE" ]]; then
        echo "!!> ERROR: Xenogears Disc 1 image not found under $ROOT/game"
        echo "    or XG_DISC. It is required for the AOT overlay build."
        exit 1
    fi
    echo "==> [full] Configuring Vita full build in $BUILD_DIR..."
    cmake -S "$ROOT" -B "$BUILD_DIR" "${CMAKE_ARGS[@]}" \
        -DXG_DISC_IMAGE="$DISC_IMAGE" \
        -DXG_RECOMPILER_EXECUTABLE="$ROOT/psxrecomp/recompiler/build/psxrecomp-game"
    echo "==> [full] Building..."
    cmake --build "$BUILD_DIR" --parallel "$PARALLEL" --target psx-runtime
    ELF="$BUILD_DIR/XenogearsRecomp"
fi

if [[ ! -f "$ELF" ]]; then
    echo "!!> ERROR: expected executable not produced: $ELF"
    exit 1
fi

echo "==> Verifying the ELF is ARM EABI32..."
MACHINE="$(arm-vita-eabi-readelf -h "$ELF" | awk '/Machine:/{print $2}')"
if [[ "$MACHINE" != "ARM" ]]; then
    echo "!!> ERROR: $ELF is not an ARM ELF (Machine: $MACHINE)."
    exit 1
fi

echo "==> Packaging VPK..."
cd "$(dirname "$ELF")"
BASE="$(basename "$ELF")"
vita-elf-create "$BASE" "$BASE.velf"
vita-make-fself "$BASE.velf" eboot.bin
vita-mksfoex -s TITLE_ID=XGEN00001 -s APP_VER=00.01 "XenogearsRecomp" app.sfo
vita-pack-vpk -s app.sfo -b eboot.bin \
    -a "$ROOT/psxrecomp/bios/openbios.bin=bios/openbios.bin" \
    -a "$ROOT/psxrecomp/bios/OpenBIOS.LICENSE=bios/OpenBIOS.LICENSE" \
    XenogearsRecomp.vpk
VPK="$(pwd)/XenogearsRecomp.vpk"

echo
echo "==> Vita build size report ($MODE mode)"
size_fmt() {
    local bytes name=$1 file=$2
    bytes="$(stat -c%s "$file")"
    printf '    %-28s %10s bytes\n' "$name" "$bytes"
}
size_fmt "ELF  ($BASE)" "$ELF"
size_fmt "VELF ($BASE.velf)" "$BASE.velf"
size_fmt "SELF (eboot.bin)" "eboot.bin"
size_fmt "VPK  (XenogearsRecomp.vpk)" "$VPK"
echo "    arm-vita-eabi-size $BASE:"
arm-vita-eabi-size "$BASE" | sed 's/^/        /'
echo "==> Done. VPK: $VPK"
