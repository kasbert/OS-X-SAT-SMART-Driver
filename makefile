all: build pkg dmg

build:
	rm -rf SATSMARTDriver/build
	(cd SATSMARTDriver; xcodebuild -configuration Release)

pkg:
	rm -rf Root
	mkdir -p Root/System/Library/Extensions/
	cp -v -r SATSMARTDriver/build/Release/SATSMARTDriver.kext Root/System/Library/Extensions/
	cp -v -r SATSMARTDriver/build/Release/SATSMARTLib.plugin Root/System/Library/Extensions/
	rm -fr satsmartdriver.pkg
	/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker --doc SATSMARTDriver.pmdoc

dmg:
	./mkdmg

