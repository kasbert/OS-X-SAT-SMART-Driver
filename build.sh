#!/bin/sh
umask 022
set -e

# /Developer/usr/bin/agvtool new-version -all 0.8

title=SATSMARTDriver
[[ "$configuration" ]] || configuration=Release
#configuration=Debug
#sdk=macosx10.5
[[ "$sdk" ]] || sdk=macosx10.6
#sdk=osx10.8

rm -rf SATSMARTDriver/build
#(cd SATSMARTDriver; xcodebuild -configuration $configuration -project SATSMARTDriver-osx10.8.xcodeproj)
#(cd SATSMARTDriver; xcodebuild -configuration $configuration -project SATSMARTDriver.xcodeproj)
#(cd SATSMARTDriver; xcodebuild -configuration $configuration -project SATSMARTDriver-$sdk.xcodeproj/ -sdk $sdk)
(cd SATSMARTDriver; xcodebuild -configuration $configuration -project SATSMARTDriver.xcodeproj/ -sdk $sdk)

rm -rf Root
mkdir -p Root/System/Library/Extensions/
ditto --rsrc SATSMARTDriver/build/$configuration/SATSMARTDriver.kext Root/System/Library/Extensions/SATSMARTDriver.kext
ditto --rsrc SATSMARTDriver/build/$configuration/SATSMARTLib.plugin Root/System/Library/Extensions/SATSMARTLib.plugin

version=$(cat SATSMARTDriver/build/$configuration/SATSMARTDriver.kext/Contents/Info.plist | xpath "//string[preceding-sibling::key[1]='CFBundleVersion']/text()")
pkg=${title}-${version}-${configuration}.pkg
[[ $configuration != Release ]] && version="$version-$configuration"
[[ $sdk != macosx10.6 ]] && version="$version-$sdk"
rm -fr "$pkg"
/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker --doc SATSMARTDriver.pmdoc --out "$pkg"

./mkdmg "$pkg" 0 "$title" $version

rm -rf "$pkg" Root

exit 0
