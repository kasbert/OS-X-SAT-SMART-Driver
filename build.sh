#!/bin/sh
umask 022
set -e

title=SATSMARTDriver
configuration=Release
#configuration=Debug

rm -rf SATSMARTDriver/build
(cd SATSMARTDriver; xcodebuild -configuration $configuration -project SATSMARTDriver.xcodeproj)
#(cd SATSMARTDriver; xcodebuild -configuration $configuration -project SATSMARTDriver-macosx10.5.xcodeproj/ -sdk macosx10.5)

rm -rf Root
mkdir -p Root/System/Library/Extensions/
ditto --rsrc SATSMARTDriver/build/$configuration/SATSMARTDriver.kext Root/System/Library/Extensions/SATSMARTDriver.kext
ditto --rsrc SATSMARTDriver/build/$configuration/SATSMARTLib.plugin Root/System/Library/Extensions/SATSMARTLib.plugin

version=$(cat SATSMARTDriver/build/$configuration/SATSMARTDriver.kext/Contents/Info.plist | xpath "//string[preceding-sibling::key[1]='CFBundleVersion']/text()")
pkg=${title}-${version}-${configuration}.pkg
[[ $configuration != Release ]] && version="$version-$configuration"
#version="$version-macosx10.5"
rm -fr "$pkg"
/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker --doc SATSMARTDriver.pmdoc --out "$pkg"

size=$(du -ks Root | awk '{print $1}')
size=$((size+100))
./mkdmg "$pkg" $size "$title" $version

rm -rf "$pkg" Root

exit 0
