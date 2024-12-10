// bme463_receiver_RAmplitudeDetection


// run file 0 to 649728 (must run in chunks of 512)
// first 10 seconds is 3600


/* ECG Simulator from SD Card - Receiver
*
* This program uses a Nucleo F303K8 to receive a short from another
* F303K8 and convert it to a float value from 0 - 1 for output from an analog
* output pin. The short sent from the other Nucleo (referred to as Sender) is
* divided by 2048 because it was obtained via an 11-bit ADC.
*
* Modified heavily from: https://forums.mbed.com/t/two-nucleo-serial-communication-via-tx-and-rx-and-vice-versa/8131
*
* Authors:    Amit J. Nimunkar and Lucas N. Ratajczyk
* Date:       05-24-2021
*
* Modified by Royal Oakes, 02-02-2022.
*/




#include "mbed.h"
#include "BME463_lib.h"




Serial      pc(USBTX,USBRX);    // Optionally - Set up serial communication with the host PC for printing statement to console
Serial      sender(D1,D0);      // Set up serial communication with the Nucleo which is sending the signal to us (Sender)
//AnalogOut   Aout(A3);           // Initialize an analog output pin to display the signal extracted from Sender




// This union is used to recieve data from the sender. Use data.s to access the
// data as a short, and use data.h to access the individual bytes of data.
typedef union _data {
  short s;
  char h[sizeof(short)];
} myShort;




myShort data;




char d;         // Variable to hold the current byte extracted from Sender
int num;        // Integer version of the myShort value
int i = 0;      // Index counter for number of bytes received
float samp_rate = 360.0f;           // Sample rate of the ISR




// Ticker for the ISR
Ticker sampTick;




// Prototypes
void ISRfxn();


int main() {
  // Set up serial communication
  sender.baud(115200);
  //pc.baud(115200); // Optional debugging.
   // Sample num at a fixed rate
  sampTick.attach(&ISRfxn, 1.0f/samp_rate);
   // Get data from sender
  while (1) {
    
      // Get the current character from Sender
      d = sender.getc();
    
      // If the byte we got was a '\0', it is possibly the terminator
      if (d == '\0' && i >= sizeof(short)){
          i = 0;                          // Reset index counter.
          num = (int) data.s;             // Convert the short to an int.
      } else if (i < sizeof(short)) {     // If 2 bytes haven't been received,
          data.h[i++] = d;                // then the byte is added to data
      }
  }
}


// Define analog outputs
// AnalogOut Aout_mwi(A4);
AnalogOut Ain_signal(A3);
// AnalogOut Aout_R_height(A4);
AnalogOut Aout_iso(A4);
AnalogOut Aout_R_amp(A5); // blue


// LPF characteristics
float a_lpf = 1.0 / pow(10, 31.13/20.0);
float cx_lpf[] = {1,2,3,4,5,6,5,4,3,2,1}; 
float cy_lpf[] = {0, 2, -1}; // Solving for y[0], so that is 0 . Have to flip b1 and b2 coefficient signs
int const nx_lpf = sizeof(cx_lpf) / sizeof(cx_lpf[0]); // Size of returns byte size
int const ny_lpf = sizeof(cy_lpf) / sizeof(cy_lpf[0]);


// HPF characteristics
float a_hpf = 1.0 / pow(10, 1.76/20.0);
float cx_hpf[] = {-0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, 0.96875, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, -0.03125, };  //
int const nx_hpf = sizeof(cx_hpf) / sizeof(cx_hpf[0]); // Size of gives you byte size
int const ny_hpf = 3;


// Derivative Filter characteristics
float a_deriv = 1.0 / 8.0;
float cx_deriv[] = {2,1,0,-1,-2};
int cy_deriv[1] = {0};
int const nx_deriv = sizeof(cx_deriv) / sizeof(cx_deriv[0]); // Size of gives you byte size
int const ny_deriv = sizeof(cy_deriv) / sizeof(cy_deriv[0]);


// Squaring Function characteristics
int cy_sq[1] = {0};
int const ny_sq = sizeof(cy_sq) / sizeof(cy_sq[0]);


// MWI characteristics
float a_mwi = 1.0 / 32;
float cx_mwi[] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
int cy_mwi[1] = {0};
int const nx_mwi = sizeof(cx_mwi) / sizeof(cx_mwi[0]); // Size of gives you byte size
int const ny_mwi = sizeof(cy_mwi) / sizeof(cy_mwi[0]);


// Define float variables for filters
float inx_lpf[nx_lpf];
float iny_lpf[ny_lpf];
float inx_hpf[nx_hpf];
float iny_hpf[ny_hpf];
float inx_deriv[nx_deriv];
float iny_deriv[ny_deriv];
float iny_sq[ny_sq];
float inx_mwi[nx_mwi];
float iny_mwi[ny_mwi];


// Define float variables for adaptive threshold
float peakt = 0;
float peaki = 0;
float thresholdi1;
float spki;
float npki;
float x5[3] = {0};
float R_height = 0;
float R_amp = 0;
float slope = 0;
float iso[8] = {0};
float avg_iso = 0;
float Ain_max = 0;
float Ain_buffer[39] = {0};


/* Interrupt function. Detects QRS Complex with Fixed Threshold.
*/
void ISRfxn() {       
   // Convert the number we extracted from Sender into a float with scale 0 - 1 (note division by 2048 due to acquisition of data by an 11-bit ADC) and output it from A3
   float fnum = (float) num/2048.0f;


   // Obtain the input
   inx_lpf[0] = fnum; // Real ECG data without known baseline, so don't subtract 0.5 to center data
   Ain_signal = inx_lpf[0];


   // LPF implementation
   shift_right(inx_lpf, nx_lpf);
   inx_lpf[0] = fnum - 0.5; // subtract 0.5 to center data
   iny_lpf[0] = filter_FIR(a_lpf, inx_lpf, cx_lpf, nx_lpf);
   // Aout_lpf = iny_lpf[0] + 0.5;


   // HPF implementation
   inx_hpf[0] = iny_lpf[0];
   shift_right(inx_hpf, nx_hpf);
   iny_hpf[0] = filter_FIR(a_hpf, inx_hpf, cx_hpf, nx_hpf);
   // Aout_hpf = iny_hpf[0] + 0.5;


   // Derivative filter implementation
   inx_deriv[0] = iny_hpf[0];
   shift_right(inx_deriv, nx_deriv); // Shift input and output to the right
   iny_deriv[0] = filter_FIR(a_deriv, inx_deriv, cx_deriv, nx_deriv);
   // Aout_deriv = iny_deriv[0] + 0.5; // add 0.5 (center at 1.65 V)


   // Squaring Function implementation
   iny_sq[0] = iny_deriv[0] * iny_deriv[0] * 80;
   // Aout_sq = iny_sq[0];


   // MWI implementation
   inx_mwi[0] = iny_sq[0];
   shift_right(inx_mwi, nx_mwi);
   iny_mwi[0] = filter_FIR(a_mwi * 20, inx_mwi, cx_mwi, nx_mwi);
   // Aout_mwi = iny_mwi[0];




   // Find all noise and signal peaks in ECG on the up slope
   x5[0] = iny_mwi[0];
   if(x5[0] > x5[2] && x5[0] > peakt) {
       // Define the temporary peak
       peakt = x5[0];
   }


   // Collect the max value of the signal
   if(fnum > Ain_max) {
       Ain_max = fnum;


       // printf("R_temp = %1.7f", (R_temp) * 3.3);
       // printf(" Volts \n\r");
   }
  
   // On the downslope, make a decision about whether the peak was a QRS complex or noise and adjust the adaptive threshold
   if(x5[0] <= x5[2] && x5[0] < 0.5 * peakt) {
       // Assign the temporary peak value to peaki
       peaki = peakt;


       if(peaki > thresholdi1) {
           // If a peak was detected, then assign the maximum value of the signal to be the height of the R wave
           R_height = Ain_max;


           // printf("R_height = %1.7f", R_height * 3.3);
           // printf(" Volts \n\r");


           // Calculate the R wave amplitude by subtracting the isopotential line from the peak value detected in the QRS complex
           R_amp = (R_height - avg_iso);


           // Print R_amp
           //pc.printf("R_height = %1.7f\n", R_height * 3.3);
           //pc.printf("avg_iso = %1.7f\n", avg_iso * 3.3);
           //pc.printf("R_amp = %1.7f", R_amp * 3.3);
           //pc.printf(" Volts \n\r");


           // Update the signal level
           spki = 0.125*peaki + 0.875*spki;
       }
       else {
           // Update the noise level
           npki = 0.125*peaki + 0.875*npki;
       }


       // Update the threshold
       thresholdi1 = npki + 0.45*(spki-npki); // NOTE: Needed to change the 0.25 to 0.45 in order to raise the threshold and avoid the second peak
       // printf("thresholdi1 = %1.7f", thresholdi1);
       // printf(" Volts \n\r");


       // Set the temporary peak value to 0 in preparation for searching for the next peak
       peakt = 0;


       // Set the max value of the signal back to 0 so that the next R peak can be detected
       Ain_max = 0;
   }


   // Create a buffer of the input signal
   Ain_buffer[0] = fnum;
   shift_right(Ain_buffer, 39);


   // Find the amplitude of the isopotential line
   slope = (Ain_buffer[0] - Ain_buffer[38]) / (0.005 * 39); // Define the slope of the signal
   if(slope <= 0.01 && slope >= -0.01 && x5[0] < thresholdi1/2 && x5[0] > -thresholdi1/2) { // If the slope is within a window of 0 and the current value of x5 is 0 +/- the threshold (to avoid reading peaks, which also occurs at a slope of 0, as the isopotential line)
       // Capture the isopotential voltage level and shift the values in the iso buffer right once
       iso[0] = fnum;
       shift_right(iso, 8);


       // Take the average of the last 8 isopotential line readings
       avg_iso = (iso[0] + iso[1] + iso[2] + iso[3] + iso[4] + iso[5] + iso[6] + iso[7]) / 8.0;


       // printf("avg_iso = %1.7f", avg_iso * 3.3);
       // printf(" Volts \n\r");
   }


   // Output the R wave amplitude to the oscilloscope
   // Aout_iso = avg_iso;
   Aout_R_amp = R_amp;


   // Shift values back in time
   shift_right(x5, 3);


}
