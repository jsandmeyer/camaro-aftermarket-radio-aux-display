### PCB files

PCB construction:

* 4-layer PCB with 5mil trace width and spacing, 10mil drill / 20mil via
* Contact finish must be ENIG due to fine-pitch components
* Use a stencil with solder paste and a hot plate or reflow oven for best results

Installation notes:

* Splice ACC power from the controller-to-radio harness (typically red wire), not direct battery (typically yellow wire).  Otherwise, your battery will drain.
* Splice low-speed GMLAN (green/white) and GND (black) from the main harness.
* Use a NEW 7-pin Molex PicoBlade cable (F-F) to connect.  DO NOT use the crossover cable that came with the trim unless you intend to re-pin the cable.  Wrap the cable in harness tape.
* You will have to remove one of the retention clips on the trim temporarily to replace the cable.
* The new circuit board should be installed in the cavity that the HVAC wires come through, below the radio.
