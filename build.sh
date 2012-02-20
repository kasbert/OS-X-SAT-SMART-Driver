#!/bin/sh
set -e
(cd SATSMARTDriver; xcodebuild -configuration Release)
cp -v -r SATSMARTDriver/build/Release/SATSMARTDriver.kext Root/System/Library/Extensions/
cp -v -r SATSMARTDriver/build/Release/SATSMARTLib.plugin Root/System/Library/Extensions/

/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker --doc SATSMARTDriver.pmdoc

