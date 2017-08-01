//MIT License
//
//Copyright (c) 2017 Angelo Pescarini (aka EEPblog)
//Based upon the "prox_volume" TouchBoard example.
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.
//===============================================================================
//HOW TO USE:
//To change the volume, hold the unlocking electrode. Now move your hand up or down
//to select your desired volume level. Once your satisfied with your choosing, let
//go of the unlocking electrode to prevent any unwanted changes to the volume.


#include <MPR121.h>         //include the necessary libraries
#include <Wire.h>
#include <HID-Project.h>    //this library can be found in the library manager
#define MPR121_ADDR 0x5C
#define MPR121_INT 4

#define LOW_DIFF -10        //global variables and fixed values
#define HIGH_DIFF 60
#define ACCEL 10     //this sets the responsivity of moving your hand, the larger the number, the more quicker it goes
#define filterWeight 0.3f // 0.0f to 1.0f - higher value = more smoothing
float lastProx = 0;
int lastOutput = 0;

#define PROX_ELECTRODE 0     //this number tells the code, where your proximity sensor is connected to
#define LOCK_ELECTRODE 2     //this number tells the code, where your unlocking electrode is connected to

void setup() {
  Serial.begin(57600);       //begin and set up all the libraries used in the code
  pinMode(LED_BUILTIN, OUTPUT); //debug
  MPR121.begin(MPR121_ADDR)
  MPR121.setInterruptPin(MPR121_INT);

  MPR121.setRegister(MPR121_NHDF, 0x01); //noise half delta (falling)
  MPR121.setRegister(MPR121_FDLF, 0x3F); //filter delay limit (falling)
  System.begin();
  Consumer.begin();
}

void loop() {                      //now here's where stuff gets interesting...
  MPR121.updateAll();              //first take  the reading of all the electrodes
  for (int i = 1; i <= 11; i++) {
    if (MPR121.isNewTouch(i)) {    //this guy here just lights up the L13 when an electrode is being touched
      digitalWrite(LED_BUILTIN, HIGH);
    } else if (MPR121.isNewRelease(i)) {
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
  if (MPR121.isNewTouch(11)) {      //if the 11th electrode was touched while taking the reading, send the PC to sleep
    System.write(SYSTEM_SLEEP);     //(it's really a secret feature, so shhhh)
  }
  if (MPR121.isNewTouch(10)) {      //if the 10th electrode was touched while taking the reading, MUTE the volume
    Consumer.press(MEDIA_VOLUME_MUTE);
  } else if (MPR121.isNewRelease(11)) {
    Consumer.release(MEDIA_VOLUME_MUTE);
  }


  if (MPR121.isNewTouch(LOCK_ELECTRODE)) {  //now if we touched the unlocking electrode, it enters another whole new loop
                                            //it's really just like you had a dream inside a dream
    while (!MPR121.isNewRelease(LOCK_ELECTRODE)) { //run this loop until we let go of the unlocking electrode
      
      MPR121.updateAll();                   //i have a feeling there is a command to update a specific electrode, but i haven't figured it out yet, so... we just update all of them
      
      unsigned int prox = constrain(MPR121.getBaselineData(PROX_ELECTRODE) - MPR121.getFilteredData(PROX_ELECTRODE), LOW_DIFF, HIGH_DIFF);
                                          //here ^ we get the proximity data by extracting the calibration data from our measured data just to make it nice and precise,
                                          //and also we constrain the value, so our uController doesn't crash when the sensor flipps out
      
      lastProx = (filterWeight * lastProx) + ((1 - filterWeight) * (float)prox); //this is where we apply our filtering, to make our data look cleaner
      
      int thisOutput = (int)map(lastProx, LOW_DIFF, HIGH_DIFF, 0, ACCEL);       //here we apply the acceleration which will dictate, by how much the volume slider
      thisOutput = constrain(thisOutput, 0, ACCEL);                             //should slide per move of hand, constrain again for error proofing
      
      Serial.println(thisOutput);                  //here the final data value is reported to serial monior for debugging
      
      if (thisOutput < lastOutput) {               //this determines, if we are moving the hand up or down based on how big is
        for (int i = 0; i <= thisOutput; i++) {    //this' loop's measured prox data and the last's loop's data
          Consumer.write(MEDIA_VOLUME_UP);
          //Serial.println("Pressing volume up");
        }
      } else if (thisOutput > lastOutput) {
        for (int i = thisOutput; i >= 0; i--) {   //the for loop here just applies the acceleration to the button presses themselves
          Consumer.write(MEDIA_VOLUME_DOWN);
          //Serial.println("Pressing volume down");
        }
      }
      lastOutput = thisOutput;                   //and finally we just save the value here for the next upcoming loop
    }
  }

}

