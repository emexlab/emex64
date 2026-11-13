#!/bin/sh

set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
PREFIX=$(cd "${1:-$HERE/../install}" && pwd)
SRC=${2:-$HERE/../llvm-project}/compiler-rt/lib/builtins
if [ ! -x "$PREFIX/bin/clang" ] || [ ! -d "$SRC" ]; then
    echo "usage: $0 [PREFIX [LLVM_SRC]]: no clang in $PREFIX/bin or no $SRC" >&2
    exit 1
fi

CC="$PREFIX/bin/clang"
AR="$PREFIX/bin/llvm-ar"
OUT="$PREFIX/lib/emex64"
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

SOURCES="
absvdi2 absvsi2 absvti2 addvdi3 addvsi3 addvti3 ashldi3 ashlti3 ashrdi3
ashrti3 bswapdi2 bswapsi2 clzdi2 clzsi2 clzti2 cmpdi2 cmpti2 ctzdi2 ctzsi2
ctzti2 divdi3 divmoddi4 divmodsi4 divmodti4 divsi3 divti3 ffsdi2 ffssi2
ffsti2 lshrdi3 lshrti3 moddi3 modsi3 modti3 muldi3 mulodi4 mulosi4 muloti4
multi3 mulvdi3 mulvsi3 mulvti3 negdi2 negti2 negvdi2 negvsi2 negvti2
paritydi2 paritysi2 parityti2 popcountdi2 popcountsi2 popcountti2 subvdi3
subvsi3 subvti3 ucmpdi2 ucmpti2 udivdi3 udivmoddi4 udivmodsi4 udivmodti4
udivsi3 udivti3 umoddi3 umodsi3 umodti3
adddf3 addsf3 comparedf2 comparesf2 divdf3 divsf3 extendsfdf2 extendhfsf2
truncdfsf2 truncdfhf2 truncsfhf2 fixdfdi fixdfsi fixdfti fixsfdi fixsfsi
fixsfti fixunsdfdi fixunsdfsi fixunsdfti fixunssfdi fixunssfsi fixunssfti
floatdidf floatdisf floatsidf floatsisf floattidf floattisf floatundidf
floatundisf floatunsidf floatunsisf floatuntidf floatuntisf muldf3 mulsf3
negdf2 negsf2 powidf2 powisf2 subdf3 subsf3 muldc3 mulsc3 divdc3 divsc3
fp_mode int_util gcc_personality_v0
"

build() {
    name=$1
    target=$2
    shift 2
    mkdir -p "$WORK/$name"
    for s in $SOURCES; do
        "$CC" --target=$target -O2 -ffreestanding -fno-builtin \
            -I"$SRC" "$@" -c "$SRC/$s.c" -o "$WORK/$name/$s.o"
    done
    mkdir -p "$(dirname "$OUT/$name")"
    rm -f "$OUT/$name.a"
    "$AR" rcs "$OUT/$name.a" "$WORK/$name"/*.o
    echo "  $OUT/$name.a"
}

build libclang_rt.builtins emex64-unknown-elf
build libclang_rt.builtins-softfp emex64-unknown-elf -msoft-float

LIBC="$HERE/libc"
CFLAGS="--target=emex64-unknown-elf -O2 -ffreestanding -nostdlibinc -I$LIBC/include"

libc() {
    name=$1
    shift
    mkdir -p "$WORK/$name"
    for s in libc.c m.c mem.S setjmp.S; do
        "$CC" $CFLAGS "$@" -c "$LIBC/$s" -o "$WORK/$name/${s%.*}.o"
    done
    rm -f "$OUT/$name.a"
    "$AR" rcs "$OUT/$name.a" "$WORK/$name"/*.o
    echo "  $OUT/$name.a"
}

libc libc
libc libc-softfp -msoft-float
"$CC" $CFLAGS -c "$LIBC/crt0.S" -o "$OUT/crt0.o"
cp "$LIBC/emex64.ld" "$OUT/emex64.ld"
rm -rf "$OUT/include"
cp -R "$LIBC/include" "$OUT/include"
echo "  $OUT/crt0.o, emex64.ld, include/"
