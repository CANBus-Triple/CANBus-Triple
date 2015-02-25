![Logo](http://canb.us/images/logo.svg)

[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/CANBus-Triple/CANBus-Triple?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

# CANBus Triple - The car hacking platform

See [CANBus Triple](http://www.canb.us) for more information, or to purchase hardware.

## About
This repository contains the base firmware and libraries for the CANBus Triple. 

[MCP2515 Library](https://github.com/CANBus-Triple/CANBus-Triple/tree/master/avr/libraries/CANBus/src) | [Examples](https://github.com/CANBus-Triple/CANBus-Triple/tree/master/avr/examples)


## Building
### Arduino IDE
This code is tested with Arduino 1.6.0. [Download](http://arduino.cc/en/Main/Software) and install the appropriate file for your platform.

### Get the code
Clone this repo / download to your Arduino [Sketchbook](http://arduino.cc/en/guide/Environment#sketchbook)/hardware folder. The path should look like this

	Documents/Ardunio/hardware/CANBus-Triple

To clone:

``git clone https://github.com/CANBus-Triple/CANBus-Triple.git``

 Or download the zip and extract it to your sketchbook folder.
 
 ![Image](http://res.cloudinary.com/ddbgan4vk/image/upload/v1423786377/download_z4of9y.jpg "") 
 
### Setup
Launch the Arduino IDE and select the newly available board variant from the menu bar under **Tools > Board > CANBus Triple**

Open the base sketch by selecting **File > Sketchbook > hardware > CANBus-Triple > avr > CANBusTriple**

This is the basic sketch that talks to the CAN Controllers [MCP2515](http://ww1.microchip.com/downloads/en/DeviceDoc/21801G.pdf), reads CAN packets from them, and routes them through the [Middleware](http://docs.canb.us/firmware/main.html#middleware-system) system.

Assure your CANBus Triple is connected to your PC via the supplied USB cable. Then select the appropriate port from **Tools > Ports**. It will probably say Arduino Leonardo beside the correct port.

Press the Build and Upload button:

![Upload](http://res.cloudinary.com/ddbgan4vk/image/upload/v1423786376/upload_hot9si.png)

The IDE will compile and upload to the CANBus Triple.

![IDE Upload](http://res.cloudinary.com/ddbgan4vk/image/upload/w_420/v1424831475/ArduinoIDEUpload_kbajva.png)



### Easy Mode

If you simply want to flash the newest firmware build you can use AVRDude or [XLoader](http://russemotto.com/xloader/) for windows to flash the appropriate .hex file to your device. The built firmware files are found in the builds folder of the repo.

Example AVRDude command:
``avrdude -Cavrdude.conf -v -patmega32u4 -cavr109 -P/dev/cu.usbmodem1411 -b57600 -D -Uflash:w:CANBusTriple.cpp.hex:i ``


## Talking over USB Serial

I use [CoolTerm](http://freeware.the-meiers.org/). It is free software available for Linux, MacOS, and Windows. Use the terminal software of your choice.

![CoolTerm](http://res.cloudinary.com/ddbgan4vk/image/upload/v1424831735/CoolTermCBT.png)


See [the docs](http://docs.canb.us/firmware/api.html) for available commands.



## Desktop / Mobile app

A hybrid web app for Android, iOS, Windows, and MacOS is in development and will be available soon.


## Now what?
Head to the [documentation site](http://docs.canb.us) for more in depth information.

Make sure to sign up at the [forum](http://forum.canb.us) for support and to engage with the community!