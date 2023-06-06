all: build pkg dmg

CONFIGURATION=Debug
SDK=macosx10.6
TITLE=SatSmartDriver
ifneq ($(CONFIGURATION),Release) 
  VERSIONPOSTFIX := $(VERSIONPOSTFIX)-$(CONFIGURATION)
endif
ifneq ($(SDK),macosx10.6)
  VERSIONPOSTFIX := $(VERSIONPOSTFIX)-$(SDK)
endif
KEXTDIR=/Library/Extensions

build:
	rm -rf SatSmartDriver/build
	(cd SatSmartDriver; xcodebuild -configuration $(CONFIGURATION) -project SatSmartDriver.xcodeproj -UseModernBuildSystem=NO)

version:
	$(eval VERSION := $(shell cat SatSmartDriver/build/$(CONFIGURATION)/SatSmartDriver.kext/Contents/Info.plist | xpath "//string[preceding-sibling::key[1]='CFBundleVersion']/text()"))
	echo VERSION $(VERSION)
	$(eval PKG := $(TITLE)-$(VERSION)-$(CONFIGURATION).pkg)
	echo PKG $(PKG)

#pkgbuild --analyze   --root Root   SatSmartDriver.plist
#productbuild --synthesize     --package SatSmartDriver.pkg    Distribution.xml

pkg: version
	rm -rf Root $(PKG)
	mkdir Root
	(cd SatSmartDriver; xcodebuild -configuration $(CONFIGURATION) -project SatSmartDriver.xcodeproj -UseModernBuildSystem=NO install DSTROOT=../Root)
	rm -f Root/usr/local/bin/smart_sample
	rm -f Root/usr/local/bin/set_properties
	mkdir -p Root/Library/Extensions/
	mv Root/System/Library/Extensions/SatSmartLib.plugin Root/Library/Extensions/
	mv Root/System/Library/Extensions/SatSmartDriver.kext Root/Library/Extensions/
	pkgbuild --root Root  --component-plist SatSmartDriver.plist --scripts Resources --identifier fi.dungeon.SatSmartDriver SatSmartDriver.pkg --install-location /usr/local
	productbuild --distribution ./Distribution.xml --package-path . $(PKG)
	#/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker --doc SatSmartDriver.pmdoc --out $(PKG)

dmg: version
	echo VERSION $(VERSION)$(VERSIONPOSTFIX)
	./mkdmg $(PKG) 0 $(TITLE) $(VERSION)$(VERSIONPOSTFIX)

unmount:
	ioreg -r -c IOSCSIPeripheralDeviceType00 -l | grep "BSD Name" | cut -d'"' -f4 | while read a; do diskutil unmountDisk "$$a" || exit 1; done

unload: unmount
	-sudo kextunload -v -b fi.dungeon.driver.SatSmartDriver
	-sudo kextunload -v -b org.dungeon.driver.SatSmartDriver

realinstall: unload
	sudo cp -R SatSmartDriver/build/$(CONFIGURATION)/SatSmartDriver.kext $(KEXTDIR)
	sudo cp -R SatSmartDriver/build/$(CONFIGURATION)/SatSmartLib.plugin $(KEXTDIR)
	sync
	sudo kextutil -t $(KEXTDIR)/SatSmartDriver.kext

install: unload
	sudo rm -rf /tmp/SatSmartDriver.kext
	sudo cp -R SatSmartDriver/build/$(CONFIGURATION)/SatSmartDriver.kext /tmp
	sudo cp -R SatSmartDriver/build/$(CONFIGURATION)/SatSmartLib.plugin $(KEXTDIR)
	sync
	sudo kextutil -t /tmp/SatSmartDriver.kext

uninstall: unload
	sudo rm -rf $(KEXTDIR)/SatSmartDriver.kext
	sudo rm -rf $(KEXTDIR)/SatSmartLib.plugin
	sudo rm -rf /tmp/SatSmartDriver.kext

clean:
	rm -rf SatSmartDriver/build
	rm -rf Root
	rm -fr SatSmartDriver-*pkg

bump-version:
	cd SatSmartDriver; /Developer/usr/bin/agvtool new-version -all 0.11
