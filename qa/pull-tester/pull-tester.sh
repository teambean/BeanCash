#!/bin/sh
# Copyright (c) 2013 The Bitcoin Core developers
# Copyright (c) 2015 Bean Core www.bitbean.org
# Copyright (c) 2017-2019 Bean Core www.beancash.org
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Helper script for pull-tester.
#Param 1: path to Bean Cash source root
#Param ...: arguments for build-test.sh

if [ $# -lt 1 ]; then
  echo "usage: $0 [Bean Cash source root] build-test arguments..."
fi

killall -q beancash-cli
killall -q beancashd

cd $1
shift

./autogen.sh
./configure
./qa/pull-tester/build-tests.sh "$@"
