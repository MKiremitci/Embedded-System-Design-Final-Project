# invoke SourceDir generated makefile for bluetooth.pem4f
bluetooth.pem4f: .libraries,bluetooth.pem4f
.libraries,bluetooth.pem4f: package/cfg/bluetooth_pem4f.xdl
	$(MAKE) -f C:\Users\CASPER-PC\Desktop\bluetoothProject\bluetooth/src/makefile.libs

clean::
	$(MAKE) -f C:\Users\CASPER-PC\Desktop\bluetoothProject\bluetooth/src/makefile.libs clean

