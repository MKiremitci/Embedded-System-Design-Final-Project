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
