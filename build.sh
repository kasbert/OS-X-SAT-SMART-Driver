#!/bin/sh
set -e
(cd SATSMARTDriver; xcodebuild -configuration Release)
rm -rf Root
mkdir -p Root/System/Library/Extensions/

cp -v -r SATSMARTDriver/build/Release/SATSMARTDriver.kext Root/System/Library/Extensions/
cp -v -r SATSMARTDriver/build/Release/SATSMARTLib.plugin Root/System/Library/Extensions/

rm -r satsmartdriver.pkg

/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker --doc SATSMARTDriver.pmdoc

#
size=550
title="SATSMARTDriver"
source=SATSMARTDriver.pkg
finalDMGName=SATSMARTDriver-0.1.dmg
applicationName=applicationName
 
#Create a R/W DMG. It must be larger than the result will be. In this example, the bash variable "size" contains the size in Kb and the contents of the folder in the "source" bash variable will be copied into the DMG:

rm -f pack.temp.dmg "$finalDMGName"

hdiutil create -srcfolder "${source}" -volname "${title}" -fs HFS+ \
      -fsargs "-c c=64,a=16,e=16" -format UDRW -size ${size}k pack.temp.dmg

#Mount the disk image, and store the device name (you might want to use sleep for a few seconds after this operation):

device=$(hdiutil attach -readwrite -noverify -noautoopen "pack.temp.dmg" | \
         egrep '^/dev/' | sed 1q | awk '{print $1}')
sleep 3

#Use AppleScript to set the visual styles (name of .app must be in bash variable "applicationName", use variables for the other properties as needed):

echo '
   tell application "Finder"
     tell disk "'${title}'"
           open
           set current view of container window to icon view
           set toolbar visible of container window to false
           set statusbar visible of container window to false
           set the bounds of container window to {400, 100, 885, 430}
           set theViewOptions to the icon view options of container window
           set arrangement of theViewOptions to not arranged
           set icon size of theViewOptions to 72
close
open
           update without registering applications
           delay 5
     end tell
   end tell
' | osascript

#           eject

#           make new alias file at container window to POSIX file "/Applications" with properties {name:"Applications"}
#           set position of item "'${applicationName}'" of container window to {100, 100}
#           set position of item "Applications" of container window to {375, 100}
#           set background picture of theViewOptions to file ".background:'${backgroundPictureName}'"

#Finialize the DMG by setting permissions properly, compressing and releasing it:

chmod -Rf go-w /Volumes/"${title}" || echo "chmod"
sync
sync
hdiutil detach ${device}
hdiutil convert "pack.temp.dmg" -format UDZO -imagekey zlib-level=9 -o "${finalDMGName}"
rm -f pack.temp.dmg 

exit 0

# Create dmg
VOL="SATSMARTDriver"
VER="0.1"
FILES="SATSMARTDriver.pkg"

DMG="tmp-$VOL.dmg"

# create temporary disk image and format, ejecting when done
SIZE=`du -sk ${FILES} | sed -n '/^[0-9]*/s/([0-9]*).*/1/p'`
SIZE=$((${SIZE}/1000+1))
#hdiutil create "$DMG" -megabytes ${SIZE} -ov -type UDIF
hdiutil create "$DMG" -megabytes ${SIZE} -ov -type UDIF -fs HFS+
DISK=`hdid "$DMG" | sed -ne ' /Apple_partition_scheme/ s|^/dev/([^ ]*).*$|1|p'`
#newfs_hfs -v "$VOL" /dev/r${DISK}s2
hdiutil eject $DISK

# mount and copy files onto volume
hdid "$DMG"
cp -R "${FILES}" "/Volumes/$VOL"
hdiutil eject $DISK
#osascript -e "tell application "Finder" to eject disk "$VOL"" && 

# convert to compressed image, delete temp image
rm -f "${VOL}-${VER}.dmg"
hdiutil convert "$DMG" -format UDZO -o "${VOL}-${VER}.dmg"
rm -f "$DMG"