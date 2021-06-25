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
KEXTDIR=/Library/Extensions

build:
	rm -rf SATSMARTDriver/build
	(cd SATSMARTDriver; xcodebuild -configuration $(CONFIGURATION) -project SATSMARTDriver.xcodeproj -UseModernBuildSystem=NO)

version:
	$(eval VERSION := $(shell cat SATSMARTDriver/build/$(CONFIGURATION)/SATSMARTDriver.kext/Contents/Info.plist | xpath "//string[preceding-sibling::key[1]='CFBundleVersion']/text()"))
	echo VERSION $(VERSION)
	$(eval PKG := $(TITLE)-$(VERSION)-$(CONFIGURATION).pkg)
	echo PKG $(PKG)

#pkgbuild --analyze   --root Root   SATSMARTDriver.plist
#productbuild --synthesize     --package SATSMARTDriver.pkg    Distribution.xml

pkg: version
	rm -rf Root $(PKG)
	mkdir Root
	(cd SATSMARTDriver; xcodebuild -configuration $(CONFIGURATION) -project SATSMARTDriver.xcodeproj -UseModernBuildSystem=NO install DSTROOT=../Root)
	rm -f Root/usr/local/bin/smart_sample
	rm -f Root/usr/local/bin/set_properties
	mkdir -p Root/Library/Extensions/
	mv Root/System/Library/Extensions/SATSMARTLib.plugin Root/Library/Extensions/
	mv Root/System/Library/Extensions/SATSMARTDriver.kext Root/Library/Extensions/
	pkgbuild --root Root  --component-plist SATSMARTDriver.plist --scripts Resources --identifier fi.dungeon.SATSMARTDriver SATSMARTDriver.pkg --install-location /usr/local
	productbuild --distribution ./Distribution.xml --package-path . $(PKG)
	#/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker --doc SATSMARTDriver.pmdoc --out $(PKG)

dmg: version
	echo VERSION $(VERSION)$(VERSIONPOSTFIX)
	./mkdmg $(PKG) 0 $(TITLE) $(VERSION)$(VERSIONPOSTFIX)

unmount:
	ioreg -r -c IOSCSIPeripheralDeviceType00 -l | grep "BSD Name" | cut -d'"' -f4 | while read a; do diskutil unmountDisk "$$a" || exit 1; done

unload: unmount
	-sudo kextunload -v -b fi.dungeon.driver.SATSMARTDriver
	-sudo kextunload -v -b org.dungeon.driver.SATSMARTDriver

realinstall: unload
	sudo cp -R SATSMARTDriver/build/$(CONFIGURATION)/SATSMARTDriver.kext $(KEXTDIR)
	sudo cp -R SATSMARTDriver/build/$(CONFIGURATION)/SATSMARTLib.plugin $(KEXTDIR)
	sync
	sudo kextutil -t $(KEXTDIR)/SATSMARTDriver.kext

install: unload
	sudo rm -rf /tmp/SATSMARTDriver.kext
	sudo cp -R SATSMARTDriver/build/$(CONFIGURATION)/SATSMARTDriver.kext /tmp
	sudo cp -R SATSMARTDriver/build/$(CONFIGURATION)/SATSMARTLib.plugin $(KEXTDIR)
	sync
	sudo kextutil -t /tmp/SATSMARTDriver.kext

uninstall: unload
	sudo rm -rf $(KEXTDIR)/SATSMARTDriver.kext
	sudo rm -rf $(KEXTDIR)/SATSMARTLib.plugin
	sudo rm -rf /tmp/SATSMARTDriver.kext

clean:
	rm -rf SATSMARTDriver/build
	rm -rf Root
	rm -fr SATSMARTDriver-*pkg

bump-version:
	cd SATSMARTDriver; /Developer/usr/bin/agvtool new-version -all 0.11
