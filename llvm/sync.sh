#!/bin/sh

set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
LLVM_SRC=${LLVM_SRC:-$HERE/llvm-project}
cd "$LLVM_SRC"

NEW="
llvm/lib/Target/Emex64
llvm/include/llvm/BinaryFormat/ELFRelocs/Emex64.def
clang/lib/Basic/Targets/Emex64.h
clang/lib/Basic/Targets/Emex64.cpp
clang/lib/CodeGen/Targets/Emex64.cpp
lld/ELF/Arch/Emex64.cpp
"

rm -rf "$HERE/overlay"
for f in $NEW; do
    mkdir -p "$HERE/overlay/$(dirname "$f")"
    cp -R "$f" "$HERE/overlay/$f"
done

mkdir -p "$HERE/patches"
git diff -- . > "$HERE/patches/emex64.patch"
echo "overlay: $(find "$HERE/overlay" -type f | wc -l) files, patch: $(grep -c '^diff' "$HERE/patches/emex64.patch") files"
