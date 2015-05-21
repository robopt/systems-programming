#!/bin/bash
# converts C constants to Rust constants
for f in $@; do
	regex='#define\s+(\w+)\s+(\w+|\(.*\))'
	egrep $regex $f |\
		sed -r 's/\/\*(.*)\*\///g;s/'$regex'/pub const \1: u32 = \2\;/g'
done
