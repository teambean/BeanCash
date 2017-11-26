#!/bin/bash
# create multiresolution windows icon
ICON_DST=../../src/qt/res/icons/beancash.ico

convert ../../src/qt/res/icons/beancash-16.png ../../src/qt/res/icons/beancash-32.png ../../src/qt/res/icons/beancash-48.png ${ICON_DST}
