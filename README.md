## Camaro Temperature & Reverse Sensor Display Shim

This describes how to get the temperature display of a PAC RPK5-GM4102 (or 4101) to function while using a Maestro RR instead of the box that came with the PAC kit.
This is useful if you want to have the Maestro functions to talk to the car, but have the nice function of the OEM-style HVAC knobs provided by the PAC kit.

### Important Legal Notices
This project is not endorsed by GM, the makers of any trim kits (PAC, iDatalink) or my aftermarket radio, any forum or their users, or anyone else, including entities named in this repository.
It is a personal project which I have completed, and I have taken a substantial risk to my own property by building and installing this project.

This project is licensed under the MIT license.
Please read the attached LICENSE document.
It applies to this documentation, all code and other documentation, and PCB designs (which are in a sense documentation and/or software themselves).
All code was written by myself, based on documentation provided by libraries or electronic part providers.
All PCB design was completed by myself.

### Background

[This forum post on camaro5.com](https://www.camaro5.com/forums/showthread.php?t=588249) goes over the process of combining the two dash kit elements.
Note the OP and others never got the temperature display to work.
This is because the trim component's harness does not have CANBUS access, and the RPK box was relaying the temperature over the Serial line.

### Goal

My goal was simply to display the temperature on the OLED.
How hard could it be?
In the process of design, I also decied to have it show backup sensor data.
The original MyLink radio showed a triangle with an exclamation point in one of five positions over the backup camera when reversing, and it would change color and flash depending on distance.
I wanted to display something similar on the OLED when reversing, because no aftermarket radios seem to support showing the backup sensor data AND camera view at the same time!

### Reverse Engineering the Hardware

I was able to do some reverse engineering of the RPK5, but without a logic analyzer or serial reading device, I could not do so completely.
I did determine that if I depinned the PAC harness to include only power input, net 5060 low-speed GMLAN, and net 7458 serial data, it would power on and the temperature would display.
However, if I disconnected the TX pin on the PAC box's internal CANBUS modem (as suggested by its documentation, would force it into "read-only" mode) it would fail completely.
I assumed this is either because doing so causes electrical mayhem within their PIC microcontroller, or theyr'e actually doing something as a preflight check that requires TX.
But, I also discovered that the OLED board used by PAC is a very standard SSD1306 SPI model, and it's roughly equivalent to something you could buy off a hobby site - and the cable connector is standard!

### Reverse Engineering the CANBUS

#### Hardware and Software

I used an Arduino Uno and the [Sparkfun CANBUS shield](https://www.sparkfun.com/products/13262) to prove viability.
With this setup, I was able to snoop on my car's low-speed CANBUS (net 5060) via the main ODB connector using pin 1 for CAN_H and pin 4 for CAN_L and GND.
Using the [mcp_can library](https://github.com/coryjfowler/MCP_CAN_lib) I was able to read temperature and such.
Some light googling got me research for CANBUS IDs for this car and for other GM cars - the data coming out is all similar, and easy to trace in logs, making the Camaro-specific IDs easy to detect.

My configuration for this was to:
* Call `CAN0.begin(MCP_STDEXT, CAN_33K3BPS, MCPHZ)` to initialize low-speed CANBUS at 33.3k.
* For masking and filtering, I set up a mask of `0x00FFE000` and filters of `0x00424000` for tempterautre and `0x003A8000` for the backup sensors.
* For the second mask, I used `0x00FF0000` and filters `0x00000000` because I don't care about the second register or other signals for this project.
* Call `CAN0.setMode(MCP_LISTENONLY)` for read-only CANBUS operation

This combination of calls and this specific library was the only way I could get filtering on extended CANBUS IDs to work.  For some reason, other options either didn't filter at all, or seemed to filter wrong.

#### Hardware V2

After confirming this configuration, I switched to using a bare ATMEGA328P chip (similar to the VQFN package I would use in production) and a MCP25625 CAN controller with transciever.
I added ferrite beads to the OLED connector after seeing that PAC decided to use them in their controller board.
I also added a switch to allow selection of Metric vs Imperial units.

#### Hardware V3

This is the production version.
I added a nice TI power supply designed via Webench, soldered everything down, and also used a jumper to select Metric vs Imperail units (in place of the switch).
Some SMT components are slightly different, like using an AMEGA328PB instead of the ATMEGA328P, or the different footprint of the oscillator.

#### Hardware V5

This is still in development.
It includes mounting holes, a power supply EMI filter (also designed by Webench), chained power supply PGOOD signals to control the OLED display, and hopefully better resiliency.

#### Logs

Temperature was easy.

For my 2014 Camaro, it comes in as 0x10424060 without mask, or 0x212 with the mask.
The second byte `buf[1]` contains the temperature... kind-of.
You have to divide the hex value by two, and subtract 40, to get degrees in celsius.


The backup sensors took more work.

I found that they were streaming data on 0x103A80BB without mask, or 0x1D4 with the mask.

The *second nibble* of the first byte `buf[0] & 0x0F` indicates the park-assist staus.
A value of `0` indicates park-assist is ON, and a value of `F` indicates it is OFF.
The *first nibble* `buf[0] & 0xF0` seems related to something else, and also seemed to vary.

The Bosch parking assist unit seems to report the direction and distance to the *closest* object only.
The second byte `buf[1]` contains the distance, in cm, from 0-255cm.

The position is more complicated.
The Bosch unit has four sensors, but reports three positional channels: Middle, Right, and Left.
It may report a single positional channel, or two adjacent channels, to form five possible locations to render the warning triangle!
The position comes out in bytes 3-4 `buf[2-3]`, split into nibbles indicating positions `[M, R], [N/A, L]`.
A value of 0 indicates the channel is off, but values of 1 thorugh 5 indicate the relative "closeness" and beep/flash speed, with 1 being closest and 5 being furthest.
If two channels are activated, such as to indicate a middle-left obstruction, both channels would have the same value.

Finally, I decided it would be prudent to have a timeout to turn off park-assist display if no data was received, in case the MCU missed the "off" signal.

### Assembly

**WARNING: DO THIS ALL AT YOUR OWN RISK.  YOU MAY DAMAGE YOUR CAR OR OTHER EQUIPMENT.  MY DESIGNS MAY HAVE FLAWS; I AM ONLY A SOFTWARE ENGINEER AFTER ALL.**

The minimum for this to work is that you need the entire PAC kit, a Maestro RR and its standard cables, and a harness to connect the Maestro RR to the camaro.
For the harness, you can use the one that comes with the complete iDatalink kit, or just the HRN-HRR-GM2 if you want to re-pin the harness.

If you use the harness with the iDatalink trim kit, just leave the HVAC connector and trim connector hanging.
For the cheap option, I originally removed pins 2, 13, 20, and 23 from the HRN-HRR-GM2 to make it seem to work.
I think I could have left 20 and 23, or changed them a little, for microphone.
If you are particularly clever, you might be able to figure out how to wire in the car's microphone to the harness, like the PAC kit does, and connect it to a 3.5mm jack for the radio.
However, for my final build, I did use the harness included in the KIT-CAM1.

I spliced into the 5060 Low-Speed GMLAN cable, the GND line, and the red "accessory power" line between the Maestro RR unit and my aftermarket radio to power the circuit board.

I used a non-crossover Molex Picoblade cable to connect the circuit board to the OLED display.
I had to temporarily remove a clip from the RPK5 trim to install the cable.
I believe the cable included in the RPK5 is a crossover cable, please do not use it for this project.

See my analysis of the various harness connector configurations in the `2014 camaro connectors.xlsx` file in this repository.
