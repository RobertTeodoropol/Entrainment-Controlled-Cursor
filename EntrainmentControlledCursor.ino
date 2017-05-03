
/*
 *
 *  Developed by Robert Teodoropol, group 7, capstone (2016-2017)
 *
 *   *** WHAT DOES IT DO ***
 *
 *  This sketch will stream data from Channel #1 only
 *  - 250 samples, corresponding to one second, are taken from the ADS manager
 *  - DC and notch filters are applied, then an RMS value is calculated for the sample
 *  - It will choose the most signicant alpha wave of five different frequencies, and act on this wave.
 *
 *
 *   *** DEPENDENCIES ***
 *   Arduino 1.6.5 was used. In order to make it work, you have to:
 *   -Get the latest ftdi drivers http://www.ftdichip.com/Drivers/VCP.htm
 *   -Install the 201 chipkit files http://chipkit.net/wiki/index.php?title=ChipKIT_core#1.29_Auto_install_via_URL_from_within_Arduino_IDE_.28latest_version_chipKIT-core_v1.3.1.29
 *   -The OpenBCI 32-bit library https://github.com/OpenBCI/OpenBCI_32bit_Library/tree/master
 *   -The OpenBCI SD libraries https://github.com/OpenBCI/OpenBCI_32bit_SD/archive/master.zip
 *   -(Modified) Biquad Libraries https://github.com/chipaudette/EEGHacker/tree/master/Arduino/OBCI_V2_AlphaDetector/Libraries
 *   Reffer to OpenBCI code on how to flash the 32-board
 *
 *
 *   *** CREDITS ***
 *
 *  This sketch uses Biquad filter library, which was originally devloped by:
 *  http://www.earlevel.com/main/2012/11/26/biquad-c-source-code/
 *
 *  The method of calucalting the Alpha power was adapted from Chip Aufette's older method for the 8-bit board.:
 *  https://github.com/chipaudette/EEGHacker/blob/master/Arduino/OBCI_V2_AlphaDetector/
 *
 *
 */

//------------------------------------------------------------------------------
//OpenBCI
#include <DSPI.h>
#include <EEPROM.h>
#include <OpenBCI_32bit_Library.h>
#include <OpenBCI_32bit_Library_Definitions.h>

//------------------------------------------------------------------------------
//   HID Bluetooth  
#include <SoftwareSerial.h>
int bluetoothTx = 13;  // TX-O pin of bluetooth mate, Arduino D2
int bluetoothRx = 17;  // RX-I pin of bluetooth mate, Arduino D3
SoftwareSerial BT(bluetoothTx, bluetoothRx);

//------------------------------------------------------------------------------
//Filters

#define MAX_N_CHANNELS (8)   //how many channels are available in hardware
#define N_EEG_CHANNELS (1)   // Currently unused, defines number of used channels incase one would want to iterate over multiple channels
//Design filters  (This BIQUAD class requires ~6K of program space!  Ouch.)
//For frequency response of these filters: http://www.earlevel.com/main/2010/12/20/biquad-calculator/
#include "Biquad.h"   //modified from this source code:  http://www.earlevel.com/main/2012/11/26/biquad-c-source-code/

// Stop DC filter
#define SAMPLE_RATE_HZ (250.0)  //default setting for OpenBCI
#define FILTER_Q (0.5)        //critically damped is 0.707 (Butterworth)
#define PEAK_GAIN_DB (0.0) //we don't want any gain in the passband
#define HP_CUTOFF_HZ (0.5)  //set the desired cutoff for the highpass filter
Biquad stopDC_filter(bq_type_highpass, HP_CUTOFF_HZ / SAMPLE_RATE_HZ, FILTER_Q, PEAK_GAIN_DB); //one for each channel because the object maintains the filter states

// Notch Filter
#define NOTCH_FREQ_HZ (60.0)      // Make sure you select the right power line frequency; set it to 60Hz if you're in the U.S.
#define NOTCH_Q (4.0)              //pretty sharp notch
Biquad notch_filter1(bq_type_notch, NOTCH_FREQ_HZ / SAMPLE_RATE_HZ, NOTCH_Q, PEAK_GAIN_DB); //one for each channel because the object maintains the filter states
Biquad notch_filter2(bq_type_notch, NOTCH_FREQ_HZ / SAMPLE_RATE_HZ, NOTCH_Q, PEAK_GAIN_DB); //one for each channel because the object maintains the filter states

// Design signal detection filter
#define BP_Q (2.0f) //somewhat steep slope
Biquad *bp1, *bp2 ;

#define MICROVOLTS_PER_COUNT (0.02235174f)  //Nov 10,2013...assumes gain of 24, includes mystery factor of 2... = 4.5/24/(2^24) *  2


//------------------------------------------------------------------------------

void setup(void) {

  // HID Bluetooth setup
  BT.begin(115200);
  BT.print("$");
  BT.print("$");
  BT.print("$");
  delay(100);
  BT.println("U,9600,N");
  BT.begin(9600);

  BT.print((char)Serial.read());


  board.useAccel = false;
  board.useAux = false;
  board.boardBegin();

  board.streamStart();
  // OPTIONAL - Turn off all channels except channel #1 -- Use Stream safe mode
  //  board.streamSafeChannelActivate(1);
  //  board.streamSafeChannelDeactivate(2);
  //  board.streamSafeChannelDeactivate(3);
  //  board.streamSafeChannelDeactivate(4);
  //  board.streamSafeChannelDeactivate(5);
  //  board.streamSafeChannelDeactivate(6);
  //  board.streamSafeChannelDeactivate(7);
  //  board.streamSafeChannelDeactivate(8);
}

void loop() {
  if (board.channelDataAvailable) {
    // Parameters of the following function are the frequencies we are interested in
    runAnalyzer(6.0f, 7.0f, 8.0f, 9.0f, 10.0f);
  }
}

float dataIn[250];
void fetchData(void) {
  for (int i = 0; i < 250 ; i++) {
    while (!(board.channelDataAvailable)) {
      delayMicroseconds(50);
    }  // wait for DRDY pin...

    board.updateChannelData();
    dataIn[i] = (float) board.boardChannelDataInt[0];
  }
}


void runAnalyzer(float f1, float f2, float f3, float f4, float f5) {

  fetchData();

  float low = 5.0f;
  float h1 = frequencyDetector(f1);
  float h2 = frequencyDetector(f2);
  float h3 = frequencyDetector(f3);
  float h4 = frequencyDetector(f4);
  float h5 = frequencyDetector(f5);

  // Find the largest value
  if ((h1 > h2) && (h1 > h3) && (h1 > h4) && (h1 > h5 ) && (h1 > low)) {
    mouseCommand(0, 10, 0);
  }
  else  if ((h2 > h3) && (h2 > h4) && (h2 > h5 ) && (h2 > low)) {
    mouseCommand(0, -10, 0);
  }
  else if ((h3 > h4) && (h3 > h5 ) && (h3 > low)) {
    mouseCommand(0, 0, -10);
  }
  else if ((h4 > h5 ) && (h4 > low)) {
    mouseCommand(0, 0, 10);
  }
  else if (h5 > low)
    mouseCommand(1, 0, 0);
}

// Instead of comparring to find the largest value, one can compare each frequency to a custom threshold
//void runAnalyzerAlternate(float f1, float f2, float f3, float f4, float f5) {
//
//  fetchData();
//
//  float low1 = 5.0f;
//  float low2 = 5.0f;
//  float low3 = 5.0f;
//  float low4 = 5.0f;
//  float low5 = 5.0f;
//  float h1 = frequencyDetector(f1);
//  float h2 = frequencyDetector(f2);
//  float h3 = frequencyDetector(f3);
//  float h4 = frequencyDetector(f4);
//  float h5 = frequencyDetector(f5);
//  if (h1 > low1) {
//    mouseCommand(0, 10, 0);
//  }
//  else  if (h2 > low2) {
//    mouseCommand(0, -10, 0);
//  }
//  else if (h3 > low3) {
//    mouseCommand(0, 0, -10);
//  }
//  else if (h4 > low4) {
//    mouseCommand(0, 0, 10);
//  }
//  else if (h5 > low5)
//    mouseCommand(1, 0, 0);
//}

//------------------------------------------------------------------------------
//Frequency Detector

float EEGHold[250];
float EEGuv = 0;
//float AHP = 0;

float frequencyDetector(float frequency) {
  float val;

  Biquad AHP_bandpass_filter1(bq_type_bandpass, frequency / SAMPLE_RATE_HZ, BP_Q, PEAK_GAIN_DB); //one for each channel because the object maintains the filter states
  Biquad AHP_bandpass_filter2(bq_type_bandpass, frequency / SAMPLE_RATE_HZ, BP_Q, PEAK_GAIN_DB); //one for each channel because the object maintains the filter states

  for (int i = 0; i < 250; i++) {

    val =  dataIn[i];

    //apply DC-blocking highpass filter
    val = stopDC_filter.process(val);    //apply DC-blocking filter

    //apply 60Hz notch filter...twice to make it more effective
    val = notch_filter1.process(val);     //apply 60Hz notch filter
    val = notch_filter2.process(val);     //apply 60Hz notch again
    //Find the power
    bp1 = &AHP_bandpass_filter1;
    bp2 = &AHP_bandpass_filter2;
    val = bp1->process(val);    //apply bandpass filter
    val = bp2->process(val);    //do it again to make it even tighter

    // Scale the data
    EEGHold[i] = val * val;

  }
  // Calculate RMS value from 250 values (One Second)
  float sum = 0.0f;
  for (int i = 0; i < 250; i++) {
    sum = sum + EEGHold[i];
  }
  sum =  sqrt(abs(sum / 250.0f));
  EEGuv = (sum * MICROVOLTS_PER_COUNT);

  return EEGuv;
}


void mouseCommand(uint8_t buttons, uint8_t x, uint8_t y) {
  BT.write(0xFD);
  BT.write((byte)0x05);
  BT.write((byte)0x02);
  BT.write(buttons);
  BT.write(x);
  BT.write(y);
  BT.write((byte)0x00);

}

