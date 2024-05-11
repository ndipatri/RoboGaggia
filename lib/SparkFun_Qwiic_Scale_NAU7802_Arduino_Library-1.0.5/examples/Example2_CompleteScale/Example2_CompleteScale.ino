/*
  Use the Qwiic Scale to read load cells and scales
  By: Nathan Seidle @ SparkFun Electronics
  Date: March 3rd, 2019
  License: This code is public domain but you buy me a beer if you use this
  and we meet someday (Beerware license).

  This example shows how to setup a scale complete with zero offset (tare),
  and linear calibration.

  If you know the calibration and offset values you can send them directly to
  the library. This is useful if you want to maintain values between power cycles
  in EEPROM or Non-volatile memory (NVM). If you don't know these values then
  you can go through a series of steps to calculate the offset and calibration value.

  Background: The IC merely outputs the raw data from a load cell. For example, the
  output may be 25776 and change to 43122 when a cup of tea is set on the scale.
  These values are unitless - they are not grams or ounces. Instead, it is a
  linear relationship that must be calculated. Remeber y = mx + b?
  If 25776 is the 'zero' or tare state, and 43122 when I put 15.2oz of tea on the
  scale, then what is a reading of 57683 in oz?

  (43122 - 25776) = 17346/15.2 = 1141.2 per oz
  (57683 - 25776) = 31907/1141.2 = 27.96oz is on the scale

  SparkFun labored with love to create this code. Feel like supporting open
  source? Buy a board from SparkFun!
  https://www.sparkfun.com/products/15242

  Hardware Connections:
  Plug a Qwiic cable into the Qwiic Scale and a RedBoard Qwiic
  If you don't have a platform with a Qwiic connection use the SparkFun Qwiic Breadboard Jumper (https://www.sparkfun.com/products/14425)
  Open the serial monitor at 115200 baud to see the output
*/

#include <Wire.h>
#include <EEPROM.h> //Needed to record user settings

#include "SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h" // Click here to get the library: http://librarymanager/All#SparkFun_NAU8702

NAU7802 myScale; //Create instance of the NAU7802 class

//EEPROM locations to store 4-byte variables
#define EEPROM_SIZE 100 //Allocate 100 bytes of EEPROM
#define LOCATION_CALIBRATION_FACTOR 0 //Float, requires 4 bytes of EEPROM
#define LOCATION_ZERO_OFFSET 10 //Must be more than 4 away from previous spot. int32_t, requires 4 bytes of EEPROM

bool settingsDetected = false; //Used to prompt user to calibrate their scale

//Create an array to take average of weights. This helps smooth out jitter.
#define AVG_SIZE 4
float avgWeights[AVG_SIZE];
byte avgWeightSpot = 0;

void setup()
{
  EEPROM.begin(EEPROM_SIZE); //Some platforms need this. Comment this line if needed

  Serial.begin(115200);
  Serial.println("Qwiic Scale Example");

  Wire.begin();
  Wire.setClock(400000); //Qwiic Scale is capable of running at 400kHz if desired

  if (myScale.begin() == false)
  {
    Serial.println("Scale not detected. Please check wiring. Freezing...");
    while (1);
  }
  Serial.println("Scale detected!");

  readSystemSettings(); //Load zeroOffset and calibrationFactor from EEPROM

  myScale.setSampleRate(NAU7802_SPS_320); //Increase to max sample rate
  myScale.calibrateAFE(); //Re-cal analog front end when we change gain, sample rate, or channel 

  Serial.print("Zero offset: ");
  Serial.println(myScale.getZeroOffset());
  Serial.print("Calibration factor: ");
  Serial.println(myScale.getCalibrationFactor());

  Serial.println("\r\nPress 't' to Tare or Zero the scale.");
}

void loop()
{
  if (myScale.available() == true)
  {
    int32_t currentReading = myScale.getReading();
    float currentWeight = myScale.getWeight();

    Serial.print("Reading: ");
    Serial.print(currentReading);
    Serial.print("\tWeight: ");
    Serial.print(currentWeight, 2); //Print 2 decimal places

    avgWeights[avgWeightSpot++] = currentWeight;
    if(avgWeightSpot == AVG_SIZE) avgWeightSpot = 0;

    float avgWeight = 0;
    for (int x = 0 ; x < AVG_SIZE ; x++)
      avgWeight += avgWeights[x];
    avgWeight /= AVG_SIZE;

    Serial.print("\tAvgWeight: ");
    Serial.print(avgWeight, 2); //Print 2 decimal places

    if(settingsDetected == false)
    {
      Serial.print("\tScale not calibrated. Press 'c'.");
    }

    Serial.println();
  }

  if (Serial.available())
  {
    byte incoming = Serial.read();

    if (incoming == 't') //Tare the scale
      myScale.calculateZeroOffset();
    else if (incoming == 'c') //Calibrate
    {
      calibrateScale();
    }
  }
}

//Gives user the ability to set a known weight on the scale and calculate a calibration factor
void calibrateScale(void)
{
  Serial.println();
  Serial.println();
  Serial.println(F("Scale calibration"));

  Serial.println(F("Setup scale with no weight on it. Press a key when ready."));
  while (Serial.available()) Serial.read(); //Clear anything in RX buffer
  while (Serial.available() == 0) delay(10); //Wait for user to press key

  myScale.calculateZeroOffset(64); //Zero or Tare the scale. Average over 64 readings.
  Serial.print(F("New zero offset: "));
  Serial.println(myScale.getZeroOffset());

  Serial.println(F("Place known weight on scale. Press a key when weight is in place and stable."));
  while (Serial.available()) Serial.read(); //Clear anything in RX buffer
  while (Serial.available() == 0) delay(10); //Wait for user to press key

  Serial.print(F("Please enter the weight, without units, currently sitting on the scale (for example '4.25'): "));
  while (Serial.available()) Serial.read(); //Clear anything in RX buffer
  while (Serial.available() == 0) delay(10); //Wait for user to press key

  //Read user input
  float weightOnScale = Serial.parseFloat();
  Serial.println();

  myScale.calculateCalibrationFactor(weightOnScale, 64); //Tell the library how much weight is currently on it
  Serial.print(F("New cal factor: "));
  Serial.println(myScale.getCalibrationFactor(), 2);

  Serial.print(F("New Scale Reading: "));
  Serial.println(myScale.getWeight(), 2);

  recordSystemSettings(); //Commit these values to EEPROM

  settingsDetected = true;
}

//Record the current system settings to EEPROM
void recordSystemSettings(void)
{
  //Get various values from the library and commit them to NVM
  EEPROM.put(LOCATION_CALIBRATION_FACTOR, myScale.getCalibrationFactor());
  EEPROM.put(LOCATION_ZERO_OFFSET, myScale.getZeroOffset());

  EEPROM.commit(); //Some platforms need this. Comment this line if needed
}

//Reads the current system settings from EEPROM
//If anything looks weird, reset setting to default value
void readSystemSettings(void)
{
  float settingCalibrationFactor; //Value used to convert the load cell reading to lbs or kg
  int32_t settingZeroOffset; //Zero value that is found when scale is tared

  //Look up the calibration factor
  EEPROM.get(LOCATION_CALIBRATION_FACTOR, settingCalibrationFactor);
  if (settingCalibrationFactor == 0xFFFFFFFF)
  {
    settingCalibrationFactor = 1.0; //Default to 1.0
    EEPROM.put(LOCATION_CALIBRATION_FACTOR, settingCalibrationFactor);
  }

  //Look up the zero tare point
  EEPROM.get(LOCATION_ZERO_OFFSET, settingZeroOffset);
  if (settingZeroOffset == 0xFFFFFFFF)
  {
    settingZeroOffset = 0; //Default to 0 - i.e. no offset
    EEPROM.put(LOCATION_ZERO_OFFSET, settingZeroOffset);
  }

  //Pass these values to the library
  myScale.setCalibrationFactor(settingCalibrationFactor);
  myScale.setZeroOffset(settingZeroOffset);

  settingsDetected = true; //Assume for the moment that there are good cal values
  if (settingCalibrationFactor == 1.0 || settingZeroOffset == 0)
    settingsDetected = false; //Defaults detected. Prompt user to cal scale.
}
