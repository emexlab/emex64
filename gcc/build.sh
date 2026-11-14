#!/bin/sh

set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
TAG=$(cat "$HERE/GCC_VERSION")
TARGET=${TARGET:-emex64e-linux-gnu}
STAGE=${STAGE:-2}
GCC_SRC=${GCC_SRC:-$HERE/gcc-src}
PREFIX=${PREFIX:-$HERE/install}
case $TARGET in
*-musl*) echo "emex64 Linux is glibc's: no $TARGET" >&2; exit 1 ;;
esac
GCC_BUILD=${GCC_BUILD:-$HERE/build}
SYSROOT=${SYSROOT:-$HERE/../linux/build-gnu/sysroot}
EMEX64_LLVM=${EMEX64_LLVM:-$HERE/../llvm/install/bin}
if [ -z "${JOBS:-}" ]; then
    JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
fi

fetch=1
[ "${1:-}" = "--no-fetch" ] && fetch=0

[ -x "$EMEX64_LLVM/clang" ] || { echo "no clang in $EMEX64_LLVM (build llvm/ first)" >&2; exit 1; }
if [ "$STAGE" != 1 ]; then
    [ -f "$SYSROOT/usr/include/stdio.h" ] ||
        { echo "no glibc sysroot at $SYSROOT (build linux/ first)" >&2; exit 1; }
fi
mkdir -p "$SYSROOT"
SYSROOT=$(cd "$SYSROOT" && pwd)

if [ ! -d "$GCC_SRC/gcc" ]; then
    [ $fetch = 1 ] || { echo "no GCC at $GCC_SRC" >&2; exit 1; }
    echo "== fetching GCC $TAG"
    git clone --depth 1 --branch "$TAG" https://github.com/gcc-mirror/gcc.git "$GCC_SRC"
fi
if [ ! -e "$GCC_SRC/gmp" ]; then
    echo "== fetching GMP, MPFR and MPC"
    (cd "$GCC_SRC" && ./contrib/download_prerequisites --no-isl)
fi

PATCH_SUM=$(cat "$HERE"/patches/*.patch | cksum | cut -d' ' -f1)
if [ "$(cat "$GCC_SRC/.emex64-applied" 2>/dev/null)" != "$PATCH_SUM" ]; then
    echo "== applying the emex64 changes"
    if [ -f "$GCC_SRC/.emex64-applied" ]; then
        (cd "$GCC_SRC" && git checkout -- .)
    fi
    (cd "$GCC_SRC" && git apply --whitespace=nowarn "$HERE"/patches/*.patch)
    echo "$PATCH_SUM" > "$GCC_SRC/.emex64-applied"
fi
cp -R "$HERE/overlay/." "$GCC_SRC/"

mkdir -p "$PREFIX/bin"
sed "s|@LLVM@|$EMEX64_LLVM|" "$HERE/tools/as" > "$PREFIX/bin/$TARGET-as"
chmod 755 "$PREFIX/bin/$TARGET-as"
ln -sf "$EMEX64_LLVM/ld.lld" "$PREFIX/bin/$TARGET-ld"
for t in ar ranlib nm objcopy objdump readelf strip size; do
    ln -sf "$EMEX64_LLVM/llvm-$t" "$PREFIX/bin/$TARGET-$t"
done
PATH=$PREFIX/bin:$PATH
export PATH

gcc_cv_as_cfi_advance_working=yes
export gcc_cv_as_cfi_advance_working

HOSTOPTS=
[ "$(uname -s)" = Darwin ] && HOSTOPTS=--with-system-zlib

COMMON="$HOSTOPTS --target=$TARGET --prefix=$PREFIX --with-sysroot=$SYSROOT
    --with-as=$PREFIX/bin/$TARGET-as --with-ld=$PREFIX/bin/$TARGET-ld
    --disable-multilib --disable-nls --disable-werror --disable-bootstrap
    --disable-libsanitizer --disable-libssp --disable-libquadmath
    --disable-libgomp --disable-libitm --disable-libvtv"

configure() {
    dir=$1
    shift
    mkdir -p "$dir"
    if [ "$(cat "$dir/.emex64-options" 2>/dev/null)" != "$*" ]; then
        echo "== configuring ($dir)"
        rm -rf "$dir"
        mkdir -p "$dir"
        (cd "$dir" && "$GCC_SRC/configure" "$@")
        echo "$*" > "$dir/.emex64-options"
    fi
}

if [ "$STAGE" = 1 ]; then
    configure "$GCC_BUILD/stage1" $COMMON --enable-languages=c --without-headers \
        --with-newlib --disable-shared --disable-threads --disable-libatomic
    echo "== building stage 1 ($JOBS jobs)"
    make -C "$GCC_BUILD/stage1" -j "$JOBS" all-gcc all-target-libgcc
    make -C "$GCC_BUILD/stage1" install-gcc install-target-libgcc
elif [ "$STAGE" = native ]; then
    [ -n "${DESTDIR:-}" ] || { echo "STAGE=native needs DESTDIR" >&2; exit 1; }
    [ -x "$PREFIX/bin/$TARGET-g++" ] ||
        { echo "no $TARGET-g++ in $PREFIX/bin: build GCC (STAGE=2) first" >&2; exit 1; }
    for d in gmp mpfr mpc isl gettext; do
        [ -d "$GCC_SRC/$d/" ] || continue
        find "$GCC_SRC/$d/" \( -name config.sub -o -name configfsf.sub \) -type f |
        while read -r f; do
            cmp -s "$GCC_SRC/config.sub" "$f" || cp "$GCC_SRC/config.sub" "$f"
        done
    done
    configure "$GCC_BUILD/native" --build="$(sh "$GCC_SRC/config.guess")" \
        --host=$TARGET --target=$TARGET --prefix=/usr --libexecdir=/usr/lib \
        --with-build-sysroot="$SYSROOT" --enable-languages=c,c++ \
        --enable-shared --enable-threads=posix --enable-__cxa_atexit \
        --with-glibc-version=2.42 --disable-multilib --disable-nls \
        --disable-werror --disable-bootstrap --disable-libsanitizer \
        --disable-libssp --disable-libquadmath --disable-libgomp \
        --disable-libitm --disable-libvtv
    echo "== building for emex64 Linux ($JOBS jobs)"
    make -C "$GCC_BUILD/native" -j "$JOBS"
    make -C "$GCC_BUILD/native" install DESTDIR="$DESTDIR"
else
    configure "$GCC_BUILD/stage2" $COMMON --enable-languages=c,c++ \
        --enable-shared --enable-threads=posix --enable-__cxa_atexit \
        --with-glibc-version=2.42
    echo "== building ($JOBS jobs)"
    make -C "$GCC_BUILD/stage2" -j "$JOBS"
    make -C "$GCC_BUILD/stage2" install
fi

echo
echo "done: $PREFIX/bin/$TARGET-gcc"
