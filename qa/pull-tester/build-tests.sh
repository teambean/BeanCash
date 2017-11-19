#!/bin/bash
# Copyright (c) 2013 The Bitcoin Core developers
# Copyright (c) 2015 Bean Core www.bitbean.org
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Param1: The prefix to mingw staging
# Param2: Path to java comparison tool
# Param3: Number of make jobs. Defaults to 1.

# Exit immediately if anything fails:
set -e
set -o xtrace

MINGWPREFIX=$1
JAVA_COMPARISON_TOOL=$2
RUN_EXPENSIVE_TESTS=$3
JOBS=${4-1}
OUT_DIR=${5-}

if [ $# -lt 2 ]; then
  echo "Usage: $0 [mingw-prefix] [java-comparison-tool] <make jobs> <save output dir>"
  exit 1
fi

DISTDIR=bitbean-1.1.0RC

# Cross-compile for windows first (breaking the mingw/windows build is most common)
cd /c/deps/bitbean-master
make distdir
mkdir -p win32-build
rsync -av $DISTDIR/ win32-build/
rm -r $DISTDIR
cd win32-build

if [ $RUN_EXPENSIVE_TESTS = 1 ]; then
  ./configure --disable-silent-rules --disable-ccache --prefix=$MINGWPREFIX --host=i586-mingw32msvc --with-qt-bindir=$MINGWPREFIX/host/bin --with-qt-plugindir=$MINGWPREFIX/plugins --with-qt-incdir=$MINGWPREFIX/include --with-boost=$MINGWPREFIX --with-protoc-bindir=$MINGWPREFIX/host/bin CPPFLAGS=-I$MINGWPREFIX/include LDFLAGS=-L$MINGWPREFIX/lib --with-comparison-tool="$JAVA_COMPARISON_TOOL"
else
  ./configure --disable-silent-rules --disable-ccache --prefix=$MINGWPREFIX --host=i586-mingw32msvc --with-qt-bindir=$MINGWPREFIX/host/bin --with-qt-plugindir=$MINGWPREFIX/plugins --with-qt-incdir=$MINGWPREFIX/include --with-boost=$MINGWPREFIX --with-protoc-bindir=$MINGWPREFIX/host/bin CPPFLAGS=-I$MINGWPREFIX/include LDFLAGS=-L$MINGWPREFIX/lib
fi
make -j$JOBS

# And compile for Linux:
cd /c/deps/bitbean-master
make distdir
mkdir -p linux-build
rsync -av $DISTDIR/ linux-build/
rm -r $DISTDIR
cd linux-build
if [ $RUN_EXPENSIVE_TESTS = 1 ]; then
  ./configure --disable-silent-rules --disable-ccache --with-comparison-tool="$JAVA_COMPARISON_TOOL" --enable-comparison-tool-reorg-tests
else
  ./configure --disable-silent-rules --disable-ccache --with-comparison-tool="$JAVA_COMPARISON_TOOL"
fi
make -j$JOBS

# link interesting binaries to parent out/ directory, if it exists. Do this before
# running unit tests (we want bad binaries to be easy to find)
if [ -d "$OUT_DIR" -a -w "$OUT_DIR" ]; then
  set +e
  # Windows:
  cp /c/deps/bitbean-master/win32-build/src/bitbeand.exe $OUT_DIR/bitbeand.exe
  cp /c/deps/bitbean-master/win32-build/src/test/test_bitbean.exe $OUT_DIR/test_bitbean.exe
  cp /c/deps/bitbean-master/win32-build/src/qt/bitbeand-qt.exe $OUT_DIR/bitbean-qt.exe
  # Linux:
  cp /c/deps/bitbean-master/linux-build/src/bitbeand $OUT_DIR/bitbeand
  cp /c/deps/bitbean-master/linux-build/src/test/test_bitbean $OUT_DIR/test_bitbean
  cp /c/deps/bitbean-master/linux-build/src/qt/bitbeand-qt $OUT_DIR/bitbean-qt
  set -e
fi

# Run unit tests and blockchain-tester on Linux:
cd /c/deps/bitbean-master/linux-build
make check

# Run RPC integration test on Linux:
/c/deps/bitbean-master/qa/rpc-tests/wallet.sh /c/deps/bitbean-master/linux-build/src
/c/deps/bitbean-master/qa/rpc-tests/listtransactions.py --srcdir /c/deps/bitbean-master/linux-build/src
# Clean up cache/ directory that the python regression tests create
rm -rf cache

if [ $RUN_EXPENSIVE_TESTS = 1 ]; then
  # Run unit tests and blockchain-tester on Windows:
  cd /c/deps/bitbean-master/win32-build
  make check
fi

# Clean up builds (pull-tester machine doesn't have infinite disk space)
cd /c/deps/bitbean-master/linux-build
make clean
cd /c/deps/bitbean-master/win32-build
make clean

# TODO: Fix code coverage builds on pull-tester machine
# # Test code coverage
# cd /c/deps/bitbean-master
# make distdir
# mv $DISTDIR linux-coverage-build
# cd linux-coverage-build
# ./configure --enable-lcov --disable-silent-rules --disable-ccache --with-comparison-tool="$JAVA_COMPARISON_TOOL"
# make -j$JOBS
# make cov
