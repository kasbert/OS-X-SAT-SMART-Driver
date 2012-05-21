#!/bin/sh
chmod -R go-w /System/Library/Extensions/SATSMARTDriver.kext /System/Library/Extensions/SATSMARTLib.plugin
chown -R root:wheel /System/Library/Extensions/SATSMARTDriver.kext /System/Library/Extensions/SATSMARTLib.plugin
kextutil /System/Library/Extensions/SATSMARTDriver.kext
exit 0
