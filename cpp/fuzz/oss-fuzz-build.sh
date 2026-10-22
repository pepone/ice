#!/bin/bash -eu
# Copyright (c) ZeroC, Inc.

# Builds the fuzz targets into $OUT for OSS-Fuzz, with the compilers and flags it sets ($CXX, $CXXFLAGS,
# $LIB_FUZZING_ENGINE, ...). To build and run them locally, use OSS-Fuzz's infra/helper.py with the ice project.

cd "$(dirname "$0")/.."

jobs=$(nproc)

# On Debian and Ubuntu, the Ice build prefixes the compiler with the GNU triplet of the target architecture
# (x86_64-linux-gnu-g++), which OSS-Fuzz's clang does not provide.
arch=$(dpkg --print-architecture)
multiarch=$(dpkg-architecture -q DEB_HOST_MULTIARCH)
compilers=("${arch}_cc=$CC" "${arch}_cxx=$CXX")

# slice2cpp is a build tool, so it is built without the fuzzing flags; it only needs the same C++ standard library
# (OSS-Fuzz builds with -stdlib=libc++).
stdlib=$(grep -o -- '-stdlib=[^ ]*' <<< "$CXXFLAGS" || true)
make -j"$jobs" "${compilers[@]}" OPTIMIZE=yes CXXFLAGS="$stdlib" LDFLAGS="$stdlib" fully_static_ldflags= src/slice2cpp

# The fuzz targets use Ice internals, which the shared library does not export.
make -j"$jobs" "${compilers[@]}" OPTIMIZE=no CONFIGS=static src/Ice

for source in fuzz/*Fuzzer.cpp; do
    # HttpParserFuzzer.cpp -> http_parser: the name of the target's seed corpus directory.
    name=$(basename "$source" Fuzzer.cpp | sed -E 's/([a-z0-9])([A-Z])/\1_\2/g' | tr '[:upper:]' '[:lower:]')
    target=ice_${name}_fuzzer
    $CXX $CXXFLAGS -std=c++17 -Iinclude -Iinclude/generated -Isrc "$source" -o "$OUT/$target" \
        $LIB_FUZZING_ENGINE "lib/$multiarch/libIce.a" -lbz2 -lssl -lcrypto -pthread
    zip -jq "$OUT/${target}_seed_corpus.zip" "fuzz/corpus/$name"/*
done
