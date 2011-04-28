#!/bin/sh

for inf in key-caps-off.svg \
		key-caps-on.svg \
		key-num-off.svg \
		key-num-on.svg \
		key-scroll-off.svg \
		key-scroll-on.svg ; do

	outf=$(basename $inf .svg).png

	inkscape -z -f $inf -e $outf -w 16 -h 16 -y 0.0 -d 90
#	convert -background none $inf -scale ${size}x${size} $outdir/$outf
done
