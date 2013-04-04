#!/bin/sh
umask 022
set -e
rm -rf SATSMARTDriver/build
(cd SATSMARTDriver; xcodebuild -configuration Release -project SATSMARTDriver.xcodeproj)
#(cd SATSMARTDriver; xcodebuild -project SATSMARTDriver-macosx10.5.xcodeproj/ -sdk macosx10.5)
rm -rf Root
mkdir -p Root/System/Library/Extensions/

ditto --rsrc SATSMARTDriver/build/Release/SATSMARTDriver.kext Root/System/Library/Extensions/SATSMARTDriver.kext
ditto --rsrc SATSMARTDriver/build/Release/SATSMARTLib.plugin Root/System/Library/Extensions/SATSMARTLib.plugin

rm -fr satsmartdriver.pkg

source=SATSMARTDriver.pkg

/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker --doc SATSMARTDriver.pmdoc --out "${source}"

#
version=$(cat SATSMARTDriver/build/Release/SATSMARTDriver.kext/Contents/Info.plist | xpath "//string[preceding-sibling::key[1]='CFBundleVersion']/text()")
size=550
title=

./mkdmg "$source" 550 "SATSMARTDriver" $version

rm -rf "$source" Root

exit 0
