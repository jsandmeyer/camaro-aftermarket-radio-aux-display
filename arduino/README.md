### Arduino Sketch

Compiled sketch size is about 28kb.

External libraries needed:

* mcp_can by coryjfowler v1.5.1
* Adafruit GFX Library 1.11.9
* Adafruit SSD1306 2.5.9

Install board variant from this link to %LocalAppData%/Arduino15/packages/arduino/hardware/avr/1.8.6/variants/pb-variant
https://github.com/MCUdude/MiniCore/blob/master/avr/variants/pb-variant/pins_arduino.h

Create a copy of the "uno" board in your boards.txt with a new designator and name, like uno_pb (name Arudino Uno PB mod).
Change the "variant" value of this copy to "pb-variant" to represent the new folder/file.

I programmed with ATMEL Ice, but an AVIRSP MkII should work too.
I used a standard FTDI interface for "debugging" via serial.
