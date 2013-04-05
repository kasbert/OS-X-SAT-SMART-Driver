all: build pkg dmg

CONFIGURATION=Debug

build:
	rm -rf SATSMARTDriver/build
	(cd SATSMARTDriver; xcodebuild -configuration $(CONFIGURATION) -project SATSMARTDriver.xcodeproj)

pkg:
	rm -rf Root
	mkdir -p Root/System/Library/Extensions/
	cp -v -r SATSMARTDriver/build/$(CONFIGURATION)/SATSMARTDriver.kext Root/System/Library/Extensions/
	cp -v -r SATSMARTDriver/build/$(CONFIGURATION)/SATSMARTLib.plugin Root/System/Library/Extensions/
	rm -fr satsmartdriver.pkg
	/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker --doc SATSMARTDriver.pmdoc

dmg:
	./mkdmg SATSMARTDriver.pkg 550 SATSMARTDriver $(CONFIGURATION)


unmount:
	ioreg -r -c IOSCSIPeripheralDeviceType00 -l | grep "BSD Name" | cut -d'"' -f4 | while read a; do diskutil unmountDisk "$$a" || exit 1; done

unload: unmount
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
	rm -fr satsmartdriver.pkg
	rm -f SATSMARTDriver*.dmg
