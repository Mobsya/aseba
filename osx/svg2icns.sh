#!/bin/bash
if [ $# -ne 2 ]; then
    echo "Usage:   svg2icns filename.svg icon.svg"
    exit 1
fi
filename="$1"
name=${filename%.*}
ext=${filename##*.}

WORK_DIR=`mktemp -d`


echo "processing: $name"
dest="$WORK_DIR"/"$name".iconset
mkdir -p "$dest"

convert -background none -resize '!16x16' "$1" "$dest/icon_16x16.png"
convert -background none -resize '!32x32' "$1" "$dest/icon_16x16@2x.png"
cp "$dest/icon_16x16@2x.png" "$dest/icon_32x32.png"
convert -background none -resize '!64x64' "$1" "$dest/icon_32x32@2x.png"
convert -background none -resize '!128x128' "$1" "$dest/icon_128x128.png"
convert -background none -resize '!256x256' "$1" "$dest/icon_128x128@2x.png"
cp "$dest/icon_128x128@2x.png" "$dest/icon_256x256.png"
convert -background none -resize '!512x512' "$1" "$dest/icon_256x256@2x.png"
cp "$dest/icon_256x256@2x.png" "$dest/icon_512x512.png"
convert -background none -resize '!1024x1024' "$1" "$dest/icon_512x512@2x.png"

iconutil -c icns "$dest" -o "$2"
rm -R "$WORK_DIR"