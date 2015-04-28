#!/bin/bash
# CC0-licensed/public domain
# see: gist.github.com/Undeterminant/9139f905904f7b29ff05
for f in $@; do
	regex='#define\s+(\w+)\s+(\w+|\(.*\))'
	egrep $regex $f |\
		sed -r 's/\/\*(.*)\*\///g;s/'$regex'/pub const \1: u32 = \2\;/g'
done
