# Embedded-System-Design-Final-Project

This repository is related to final project of Embedded System Design lecture in Eskişehir Technical University. 

The topic of this project is implementing a small messaging app which uses Bluetooth HC06 module, SW-EK-TM4C1294XL Tiva C Launchpad and a computer. Flow of the message is shown in the below figure. 
1. User write messages in Termite. 
1. The message send Launchpad via Bluetooth by using UART concept.
1. Received message from PC is manipulate in the program and then transmitted to TCP server via TCP socket.
1. Hercules print the received message from launcpad via TCP socket on the screen.

![alt text](https://github.com/MKiremitci/Embedded-System-Design-Final-Project/blob/main/flowMessage.png)

## Requirements
1. Code Composer Studio 10.0
1. TI-RTOS for Tiva C
1. User can be inlucude all libraries in the codes with successfully! 
1. Termite 3.4
1. Hercules 3.2.8
1. Bluetooth HC06 or HC05 (HC05 canbe used as a slave and master, while HC06 can be used as slave only.)
1. Ethernet cable 
1. Two F-F Jumpers and Two M-F Jumpers
1. EK-TM4C1294XL Launcpad


## How to import project into Code Composer Studio?
1. Unzip the "EEM449-Final Project- Mert Kiremitci.zip"
1. Start Code Composer Studio 10.0 
1. File
1. Open Projects from File System
1. Directory...
1. Find the unzipped folder which is named as EEM449-Final Project- Mert Kiremitci and select it.
1. Finish

## Some problems and their solutions
#### I cannot receive NTP timestampt from server? 
* You can try another IP adresses which can be found in https://tf.nist.gov/tf-cgi/servers.cgi
* Turn off any antivirus program and Windows Defender, then try again.
* Check ethernet connection for launcpad and internet connection for computer. 

#### Termite is not responding.
* Probably, you are not using true COM port. You should select true COM port. This port number change computer to computer, thus you should find yourself.

#### I cannot transmit message to the server? 
* Probably, you are not sending messages to the true port number. Careful! You should write the true port number to the SOCKET_PORT in blueetooth.c
* Check ethernet connection for launcpad and internet connection for computer. 
* Don't forget that computer and launcpad must be connected to the same network!
* SOCKET_IP adress must be local address of your computer. In order to learn your computer local adress, you can type "ipconfig" in CMD.
