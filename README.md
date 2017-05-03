# Entrainment-Controlled-Cursor
Developed by Robert Teodoropol, group 7, capstone (2016-2017) for The Neil Squire Society http://www.neilsquire.ca/
The following is the code used for an entrainment controlled cursor developed by group 7 in their 2016-2017 capstone for the NeilSquireSociety.
It was developed for the 32-bit OpenBCI cyton board in arduino 1.6.5. 
All libraries used are included in the library folder, additionally, links to the original sources are provided in below Cursor.

A BlueSmurf RN-42 was used, as well was the OpenBCI cyton board. An arduino leonoardo was used to flash an LED at varying frequencies and brightness

<b>WHAT DOES IT DO</b>
   This sketch will stream data from Channel #1 only
  - 250 samples, corresponding to one second, are taken from the ADS manager
  - DC and notch filters are applied, then an RMS value is calculated for the sample
  - It will choose the most significant alpha wave of five different frequencies, and act on this wave.

<b>DEPENDENCIES</b> 
   Arduino 1.6.5 was used. In order to make it work, you have to:
  - Get the latest ftdi drivers http://www.ftdichip.com/Drivers/VCP.htm
  - Install the 201 chipkit files http://chipkit.net/wiki/index.php?title=ChipKIT_core#1.29_Auto_install_via_URL_from_within_Arduino_IDE_.28latest_version_chipKIT-core_v1.3.1.29
  - The OpenBCI 32-bit library from this git, or from https://github.com/OpenBCI/OpenBCI_32bit_Library/tree/master
  - The OpenBCI SD libraries from this git, or https://github.com/OpenBCI/OpenBCI_32bit_SD/archive/master.zip
  - (Modified) Biquad Libraries from this git, or https://github.com/chipaudette/EEGHacker/tree/master/Arduino/OBCI_V2_AlphaDetector/Libraries
  - Refer to OpenBCI documentation for any more information on how to flash the 32-board: http://docs.openbci.com

<b>CREDITS</b>
   This capstone project was done at the University of British Columbia, under the supervision of Dr.Elnaggar, for The Neil Squire Society http://www.neilsquire.ca/
   This sketch uses Biquad filter library, which was originally developed by: http://www.earlevel.com/main/2012/11/26/biquad-c-source-code/
   The method of calculating the Alpha power was adapted from Chip Audette's older method for the 8-bit board.: https://github.com/chipaudette/EEGHacker/blob/master/Arduino/OBCI_V2_AlphaDetector/
	 
	 Tip: To simulate real-life measurements, an OpenBCI input channel was put in a breadboard, across the divide from a flashing LED, using analogWrite(32); for an approximate measurement
