OS X SAT SMART Driver
---------------------

This is a kernel driver for Mac OS X external USB or FireWire drives.
It extends the standard driver behaviour by providing access to drive
SMART data. The interface to SMART data is same as with ATA family
driver, so most existing applications should work. 

The driver requires a SAT (SCSI ATA Translation) capable external
drive enclosure. Check smartmontools list for supported devices:
http://smartmontools.org/wiki/Supported_USB-Devices 
If the Option column contains sat or jmicron, the driver should work.
The driver should work with Snow Leopard and Lion and Mountain Lion. 
People have reported problems with Lion and Encrypted volumes. 
Some enclosures are reported to work with FireWire but not with USB.
The driver is not compatible to WD Drive Manager, or enclosures 
with custom kernel extensions.

Install
-------
 
 * Unmount external drives
 * Use the dmg image and the installer from [Releases](https://github.com/Artoria2e5/SatSmartDriver_kext/Releases)
 * Check DiskUtility. The disks should have "S.M.A.R.T. Status: Verified"


### Note for Yosemite (Mac OS X 10.10) and above users

Apple now wants us to sign drivers and extorts a very high fee for it.
Now there's no chance we will be paying for it, so disable the check if
you can.

On Yosemite, use:

    sudo nvram boot-args-"kext-dev-mode-1"

On newer macOS, try (must be run in Recovery OS):

    csrutil enable --without kext

If you are using [OpenCore](https://github.com/acidanthera/OpenCorePkg),
you can move the kext file from its standard install location
(`/Library/Extensions/`) to the EFI OpenCore directory and load it the OC way
by editing the config file, `config.plist`.

The `.plugin` file is only used to support the example client. `smartctl`
does not use it.


Uninstall
---------

 * Remove driver and plugin
   ```
   sudo rm -r /System/Library/Extensions/SATSMARTDriver.kext
   sudo rm -r /System/Library/Extensions/SATSMARTLib.plugin
   sudo rm -r /Library/Extensions/SATSMARTDriver.kext
   sudo rm -r /Library/Extensions/SATSMARTLib.plugin
   ```
 * Reboot


Developer information
---------------------

### Structure

This driver creates a pretend ATA device over USB or FireWire. As a result,
`smartctl` will *not* be able to use the `-d sat` option to access the device;
instead, it will use the `-d ata` option.

You will also not be able to use `smartctl`'s USB device database to decide which
protocol to use. Instead, the driver will choose a protocol according to its
own database, `SATSMARTDriver.kext/Contents/Info.plist`. Because plists are plists,
the USB IDs are stored in decimal, not the usual hexadecimal. This database is
quite outdated and I hope to find a way to generate it automatically from the
smartmontools database.

`smartctl` should remain able to detect the model and use model-appropriate
interpretations of raw SMART values.

The `.plugin` file is *only* a library for the demo programs.  You will not need
it.

### Compile

 * Compile all targets
   ```
   make pkg
   ```

 * Unmount all existing external drives.

 * Load the kernel extension.
   ```
   sudo tail -f /private/var/log/kernel.log &
   sudo make install
   ```

 * The external drives should mount automatically.

 * Test
   ```
   cd SATSMARTDriver/build/Debug/
   ./smart_status
   ./smart_sample -a
   ```

 * Check DiskUtility. The disk should have "S.M.A.R.T. Status: Verified"

 * Install the driver to system permanently
    sudo make realinstall

 * Reboot

 * If you want to limit the driver for certain hardware add product 
   and vendor identification to SATSMARTDriver/Info.plist.
   Check chapter "Adding enclosure identification"


### Adding enclosure identification

Check, if the enclosure is identified using shell:

```bash-session
ioreg -r -w 0 -c fi_dungeon_driver_IOSATDriver  | egrep 'Enclosure|PassThroughMode|Capable'| grep -v Identifiers
  |   "Enclosure Name" - "Unknown 0928:000a"
  |   "SATSMARTCapable" - Yes
  |   "PassThroughMode" - "sat16"
```

Add an issue to GitHub.

FAQ
---

### NVMe?

Not possible in this driver, ever.  Someone can write a pretend-NVMe driver
some day.

(On a normal system, `smartctl` should handle all of this with an SCSI
passthrough API.  macOS is not a normal system so you can't go around shooting
SCSI commands from userspace.  Code duplication, yay!)

License
-------

The code is based on Apple opensource files and is therefore published
under Apple Public Source License. For details see
http://www.opensource.apple.com/license/apsl/
