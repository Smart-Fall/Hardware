// Custom chips playground
// See https://link.wokwi.com/custom-chips-alpha for more info


#include <Wire.h>                // I2C  Library
#include "MAX30105.h"            // MAX3010x library
#include "heartRate.h"           // Heart rate calculating algorithm


MAX30105 particleSensor;

const byte RATE_SIZE = 4;        // Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE];           // Array of heart rates
byte rateSpot = 0;
long lastBeat = 0;               // Time at which the last beat occurred
float beatsPerMinute;
int beatAvg;


void setup() {
  // Initialize sensor
  Serial.begin(9600);
  particleSensor.begin(Wire, I2C_SPEED_FAST);    // Use default I2C port, 400kHz speed
  particleSensor.setup();                        // Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A);     // Turn Red LED to low to indicate sensor is running
  Serial.println("Initializing sensor");
}


void loop() {
  long irValue = particleSensor.getIR();              // Reading the IR value it will permit us to know if there's a finger on the sensor or not
  irValue = 50050;  
  if (irValue > 50000)
  {

    if (checkForBeat(irValue) == true)                  //If a heart beat is detected, call checkForBeat as frequent as possible to get accurate value
    {
      long delta = millis() - lastBeat;                 //Measure duration between two beats
      lastBeat = millis();
      beatsPerMinute = 60 / (delta / 1000.0);           //Calculating the BPM
      if (beatsPerMinute < 255 && beatsPerMinute > 20)  //To calculate the average we strore some values (4) then do some math to calculate the average
      {
        rates[rateSpot++] = (byte)beatsPerMinute;       //Store this reading in the array
        rateSpot %= RATE_SIZE;                          //Wrap variable

        //Take average of readings
        beatAvg = 0;
        for (byte x = 0 ; x < RATE_SIZE ; x++)
          beatAvg += rates[x];
        beatAvg /= RATE_SIZE;

        Serial.println("BPM:" + beatAvg);
      }
    }
  }
  else
  {
    Serial.println("No finger detected");
  }
}

