#!/bin/sh
# Copyright (c) 2013 The Bitcoin Core developers
# Copyright (c) 2015 Bean Core www.bitbean.org
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Helper script for pull-tester.
#Param 1: path to bitbean srcroot
#Param ...: arguments for build-test.sh

if [ $# -lt 1 ]; then
  echo "usage: $0 [bitbean srcroot] build-test arguments..."
fi

killall -q bitbean-cli
killall -q bitbeand

cd $1
shift

./autogen.sh
./configure
./qa/pull-tester/build-tests.sh "$@"
