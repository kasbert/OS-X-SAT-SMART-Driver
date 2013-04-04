#!/bin/sh
umask 022
set -e

title=SATSMARTDriver
pkg=SATSMARTDriver.pkg
configuration=Release

rm -rf SATSMARTDriver/build
(cd SATSMARTDriver; xcodebuild -configuration $configuration -project SATSMARTDriver.xcodeproj)
#(cd SATSMARTDriver; xcodebuild -project SATSMARTDriver-macosx10.5.xcodeproj/ -sdk macosx10.5)

rm -rf Root
mkdir -p Root/System/Library/Extensions/
ditto --rsrc SATSMARTDriver/build/$configuration/SATSMARTDriver.kext Root/System/Library/Extensions/SATSMARTDriver.kext
ditto --rsrc SATSMARTDriver/build/$configuration/SATSMARTLib.plugin Root/System/Library/Extensions/SATSMARTLib.plugin

rm -fr "$pkg"
/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker --doc SATSMARTDriver.pmdoc --out "$pkg"

version=$(cat SATSMARTDriver/build/$configuration/SATSMARTDriver.kext/Contents/Info.plist | xpath "//string[preceding-sibling::key[1]='CFBundleVersion']/text()")
[[ $configuration != Release ]] && version="$version-$configuration"
./mkdmg "$pkg" 550 "$title" $version

rm -rf "$pkg" Root

exit 0
