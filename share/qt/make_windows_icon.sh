#!/bin/bash
# create multiresolution windows icon
ICON_DST=../../src/qt/res/icons/bitbean.ico

convert ../../src/qt/res/icons/bitbean-16.png ../../src/qt/res/icons/bitbean-32.png ../../src/qt/res/icons/bitbean-48.png ${ICON_DST}
