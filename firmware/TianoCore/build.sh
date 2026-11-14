#!/bin/bash

set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
TAG=$(cat "$HERE/EDK2_VERSION")
EDK2_SRC=${EDK2_SRC:-$HERE/edk2}
EMEX64_LLVM=${EMEX64_LLVM:-$HERE/../../llvm/install/bin}
if [ -z "${JOBS:-}" ]; then
    JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
fi

fetch=1
TARGET=DEBUG
for arg in "$@"; do
    case "$arg" in
        --no-fetch) fetch=0 ;;
        DEBUG|RELEASE|NOOPT) TARGET=$arg ;;
        *) echo "usage: $0 [--no-fetch] [DEBUG|RELEASE|NOOPT]" >&2; exit 1 ;;
    esac
done

if [ ! -x "$EMEX64_LLVM/clang" ] || [ ! -x "$EMEX64_LLVM/ld.lld" ]; then
    echo "no emex64 clang in $EMEX64_LLVM: run ../../llvm/build.sh or set EMEX64_LLVM" >&2
    exit 1
fi
EMEX64_LLVM=$(cd "$EMEX64_LLVM" && pwd)

if [ ! -d "$EDK2_SRC/MdePkg" ]; then
    [ $fetch = 1 ] || { echo "no edk2 at $EDK2_SRC" >&2; exit 1; }
    echo "== fetching EDK2 $TAG"
    git clone --depth 1 --branch "$TAG" https://github.com/tianocore/edk2.git "$EDK2_SRC"
fi

echo "== submodules"
(cd "$EDK2_SRC" && git submodule update --init --depth 1 \
    BaseTools/Source/C/BrotliCompress/brotli \
    MdeModulePkg/Library/BrotliCustomDecompressLib/brotli \
    MdePkg/Library/MipiSysTLib/mipisyst \
    MdePkg/Library/BaseFdtLib/libfdt)

PATCH_SUM=$(cat "$HERE"/patches/*.patch | cksum | cut -d' ' -f1)
if [ "$(cat "$EDK2_SRC/.emex64-applied" 2>/dev/null)" != "$PATCH_SUM" ]; then
    echo "== applying the emex64 changes"
    if [ -f "$EDK2_SRC/.emex64-applied" ]; then
        (cd "$EDK2_SRC" && git checkout -- . && git clean -fdq -e .emex64-applied)
    fi
    (cd "$EDK2_SRC" && git apply --whitespace=nowarn "$HERE"/patches/*.patch)
    echo "$PATCH_SUM" > "$EDK2_SRC/.emex64-applied"
fi

cp -R "$HERE/overlay/." "$EDK2_SRC/"

LOGO_BMP=$EDK2_SRC/MdeModulePkg/Logo/Logo.bmp
if [ -f "$HERE/logo.png" ]; then
    python3 "$EDK2_SRC/Emex64Pkg/Tools/png2bmp.py" "$HERE/logo.png" "$LOGO_BMP.new"
else
    (cd "$EDK2_SRC" && git show HEAD:MdeModulePkg/Logo/Logo.bmp) > "$LOGO_BMP.new"
fi
if ! cmp -s "$LOGO_BMP.new" "$LOGO_BMP"; then
    mv "$LOGO_BMP.new" "$LOGO_BMP"
    rm -rf "$EDK2_SRC"/Build/Emex64/*/EMEX64/MdeModulePkg/Logo
else
    rm -f "$LOGO_BMP.new"
fi

echo "== BaseTools"
make -C "$EDK2_SRC/BaseTools/Source/C" -j "$JOBS"

echo "== TianoCore ($TARGET, $JOBS jobs)"
cd "$EDK2_SRC"
export WORKSPACE="$EDK2_SRC"
export EDK_TOOLS_PATH="$EDK2_SRC/BaseTools"
export EMEX64_LLVM_BIN="$EMEX64_LLVM/"
export EMEX64_RC="$EDK2_SRC/Emex64Pkg/Tools/emex64-rc"
set +u
. ./edksetup.sh --reconfig
set -u
build -a EMEX64 -t CLANGEMEX64 -p Emex64Pkg/Emex64.dsc -b "$TARGET" -n "$JOBS"

cp "$EDK2_SRC/Build/Emex64/${TARGET}_CLANGEMEX64/FV/TIANOCORE.fd" "$HERE/firmware.img"
echo
echo "done: $HERE/firmware.img"
