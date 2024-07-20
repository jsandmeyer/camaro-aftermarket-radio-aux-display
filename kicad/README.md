### PCB files

PCB construction:

* 4-layer PCB with 5mil trace width and spacing support, and standard drill limits
* Contact finish must be ENIG fdue to fine-pitch components
* Use a stencil with solder paste and a reflow oven or hot plate for best results, especially due to QFN packages

V3: original design, no mounting holes and no EMI filter.  Boots up about 98% of the time.

V5: new design, but still in progress and untested.  Change notes:

* Added EMI filter based on webench design
* Added mounting holes
* 3.3V power supply replaced with model with more features
* 3.3V power supply will wait for 5V PGOOD signal
* OLED RESET controller removed; reset pin now feeds from MCU
* Level shifter is repalced with a device with more pins
* Level shifter is disabled until 3.3V PGOOD signal

Installation notes:

* Splice ACC power from the brain-to-radio harness (typically red wire), not direct battery (typically yellow wire).  Otherwise, your battery will drain.
* Splice low-speed GMLAN (green/white) and GND (black) from the main harness.
* Use a long, standard Molex PicoBlade cable (F-F) to connect.  Do not use the crossover cable that came with the trim.  Wrap the cable in harness tape.
* You will have to remove one of the tabs on the trim temporarily to replace the cable.
* The new circuit board should be installed in the cavity that the HVAC wires come through, below the radio.
