all: build pkg dmg

CONFIGURATION=Debug
SDK=macosx10.6
TITLE=SATSMARTDriver
ifneq ($(CONFIGURATION),Release) 
  VERSIONPOSTFIX := $(VERSIONPOSTFIX)-$(CONFIGURATION)
endif
ifneq ($(SDK),macosx10.6)
  VERSIONPOSTFIX := $(VERSIONPOSTFIX)-$(SDK)
endif

build:
	rm -rf SATSMARTDriver/build
	(cd SATSMARTDriver; xcodebuild -configuration $(CONFIGURATION) -project SATSMARTDriver.xcodeproj)

version:
	$(eval VERSION := $(shell cat SATSMARTDriver/build/$(CONFIGURATION)/SATSMARTDriver.kext/Contents/Info.plist | xpath "//string[preceding-sibling::key[1]='CFBundleVersion']/text()"))
	echo VERSION $(VERSION)
	$(eval PKG := $(TITLE)-$(VERSION)-$(CONFIGURATION).pkg)
	echo PKG $(PKG)

pkg: version
	rm -rf Root $(PKG)
	mkdir -p Root/System/Library/Extensions/
	ditto --rsrc SATSMARTDriver/build/$(CONFIGURATION)/SATSMARTDriver.kext Root/System/Library/Extensions/SATSMARTDriver.kext
	ditto --rsrc SATSMARTDriver/build/$(CONFIGURATION)/SATSMARTLib.plugin Root/System/Library/Extensions/SATSMARTLib.plugin
	/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker --doc SATSMARTDriver.pmdoc --out $(PKG)

dmg: version
	echo VERSION $(VERSION)$(VERSIONPOSTFIX)
	./mkdmg $(PKG) 0 $(TITLE) $(VERSION)$(VERSIONPOSTFIX)

unmount:
	ioreg -r -c IOSCSIPeripheralDeviceType00 -l | grep "BSD Name" | cut -d'"' -f4 | while read a; do diskutil unmountDisk "$$a" || exit 1; done

unload: unmount
	-sudo kextunload -v -b fi.dungeon.driver.SATSMARTDriver
	-sudo kextunload -v -b org.dungeon.driver.SATSMARTDriver

realinstall: unload
	sudo cp -R SATSMARTDriver/build/$(CONFIGURATION)/SATSMARTLib.plugin /System/Library/Extensions
	sudo cp -R SATSMARTDriver/build/$(CONFIGURATION)/SATSMARTDriver.kext /System/Library/Extensions
	sync
	sudo kextutil -t /System/Library/Extensions/SATSMARTDriver.kext

install: unload
	sudo cp -R SATSMARTDriver/build/$(CONFIGURATION)/SATSMARTLib.plugin /System/Library/Extensions
	sudo rm -rf /tmp/SATSMARTDriver.kext
	sudo cp -R SATSMARTDriver/build/$(CONFIGURATION)/SATSMARTDriver.kext /tmp
	sync
	sudo kextutil -t /tmp/SATSMARTDriver.kext

uninstall: unload
	sudo rm -rf /System/Library/Extensions/SATSMARTDriver.kext
	sudo rm -rf /System/Library/Extensions/SATSMARTLib.plugin
	sudo rm -rf /tmp/SATSMARTDriver.kext

clean:
	rm -rf SATSMARTDriver/build
	rm -rf Root
	rm -fr SATSMARTDriver-*pkg

bump-version:
	cd SATSMARTDriver; /Developer/usr/bin/agvtool new-version -all 0.10
