# Make png based spinner for Bean Cash GUI (replaces mng format)
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Distributed under the MIT software license, see accompanying file COPYING or https://www.opensource.org/licenses/mit-license.php
# Copyright (c) 2021 Bean Core www.beancash.org

FRAMEDIR=$(dirname $0)
for i in {0..34}
do
	frame=$(printf "%03d" $i)
	angle=$(($i * 20))
	convert $FRAMEDIR/../src/spinnerbean.png -background "rgba(0,0,0,0.0)" +level-colors 'rgb(55,106,0)' -distort SRT $angle $FRAMEDIR/spinnerbean-$frame.png
done


