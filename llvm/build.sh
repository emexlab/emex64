set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
TAG=$(cat "$HERE/LLVM_VERSION")
LLVM_SRC=${LLVM_SRC:-$HERE/llvm-project}
LLVM_BUILD=${LLVM_BUILD:-$HERE/build}
PREFIX=${PREFIX:-$HERE/install}
if [ -z "${JOBS:-}" ]; then
    JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
fi

fetch=1
[ "${1:-}" = "--no-fetch" ] && fetch=0

if [ ! -d "$LLVM_SRC/llvm" ]; then
    [ $fetch = 1 ] || { echo "no llvm-project at $LLVM_SRC" >&2; exit 1; }
    echo "== fetching LLVM $TAG"
    git clone --depth 1 --branch "$TAG" https://github.com/llvm/llvm-project.git "$LLVM_SRC"
fi

PATCH_SUM=$(cat "$HERE"/patches/*.patch | cksum | cut -d' ' -f1)
if [ "$(cat "$LLVM_SRC/.emex64-applied" 2>/dev/null)" != "$PATCH_SUM" ]; then
    echo "== applying the emex64 changes"
    if [ -f "$LLVM_SRC/.emex64-applied" ]; then
        (cd "$LLVM_SRC" && git checkout -- .)
    fi
    (cd "$LLVM_SRC" && git apply --whitespace=nowarn "$HERE"/patches/*.patch)
    echo "$PATCH_SUM" > "$LLVM_SRC/.emex64-applied"
fi

cp -R "$HERE/overlay/." "$LLVM_SRC/"

echo "== configuring"
cmake -G Ninja -S "$LLVM_SRC/llvm" -B "$LLVM_BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DLLVM_ENABLE_PROJECTS="clang;lld" \
    -DLLVM_TARGETS_TO_BUILD="" \
    -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=Emex64 \
    -DLLVM_DEFAULT_TARGET_TRIPLE=emex64-unknown-elf \
    -DLLVM_ENABLE_ASSERTIONS=ON \
    -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_BENCHMARKS=OFF \
    -DLLVM_INCLUDE_EXAMPLES=OFF -DCLANG_INCLUDE_TESTS=OFF \
    -DLLVM_ENABLE_ZLIB=OFF -DLLVM_ENABLE_ZSTD=OFF -DLLVM_ENABLE_LIBXML2=OFF \
    -DCLANG_ENABLE_ARCMT=OFF -DCLANG_ENABLE_STATIC_ANALYZER=OFF \
    -DLLVM_INSTALL_UTILS=ON \
    -DLLVM_DISTRIBUTION_COMPONENTS="clang;clang-resource-headers;lld;llc;llvm-mc;llvm-objdump;llvm-readobj;llvm-readelf;llvm-ar;llvm-ranlib;llvm-nm;llvm-objcopy;llvm-strip;llvm-size;llvm-dwarfdump;FileCheck;not"

echo "== building ($JOBS jobs)"
ninja -C "$LLVM_BUILD" -j "$JOBS" install-distribution

echo "== the emex64 runtime (compiler-rt builtins)"
"$HERE/runtime/build.sh" "$PREFIX" "$LLVM_SRC"

echo
echo "done: $PREFIX/bin/clang --target=emex64-unknown-elf"
