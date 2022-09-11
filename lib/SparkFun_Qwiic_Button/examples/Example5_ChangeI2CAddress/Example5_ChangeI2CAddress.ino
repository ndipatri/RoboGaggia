/******************************************************************************
  A configurator for changing the I2C address on the Qwiic Button/Switch that walks
  the user through finding the address of their button, and then changing it!

  Fischer Moseley @ SparkFun Electronics
  Original Creation Date: July 30, 2019

  This code is Lemonadeware; if you see me (or any other SparkFun employee) at the
  local, and you've found our code helpful, please buy us a round!

  Hardware Connections:
  Attach the Qwiic Shield to your Arduino/Photon/ESP32 or other
  Plug the button into the shield
  Print it to the serial monitor at 115200 baud.

  Distributed as-is; no warranty is given.
******************************************************************************/

#include <SparkFun_Qwiic_Button.h>
QwiicButton button;

void setup() {
  Serial.begin(115200);
  Serial.println("Qwiic button examples");
  Wire.begin(); //Join I2C bus

  //check if button will acknowledge over I2C
  if (button.begin() == false) {
    Serial.println("Device did not acknowledge! Running scanner.");
  }
  else{
    Serial.println("Device acknowledged!");
  
    Serial.println();
    Serial.println("Enter a new I2C address for the Qwiic Button/Switch to use!");
    Serial.println("Don't use the 0x prefix. For instance, if you wanted to");
    Serial.println("change the address to 0x5B, you would enter 5B and press enter.");
    Serial.println();
    Serial.println("One more thing! Make sure your line ending is set to 'Both NL & CR'");
    Serial.println("in the Serial Monitor.");
    Serial.println();

    //Wait for serial data to be available
    while (Serial.available() == 0);
  
    if (Serial.available()) {
      uint8_t newAddress = 0;
      String stringBuffer = Serial.readString();
      char charBuffer[10];
      stringBuffer.toCharArray(charBuffer, 10);
      uint8_t success = sscanf(charBuffer, "%x", &newAddress);
  
      if (success) {
        if (newAddress > 0x08 && newAddress < 0x77) {
          Serial.println("Character recieved, and device address is valid!");
          Serial.print("Attempting to set device address to 0x");
          Serial.println(newAddress, HEX);
  
          if (button.setI2Caddress(newAddress) == true) {
            Serial.println("Device address set succeeded!");
          }
  
          else {
            Serial.println("Device address set failed!");
          }
  
          delay(100); //give the hardware time to do whatever configuration it needs to do
  
          if (button.isConnected()) {
            Serial.println("Device will acknowledge on new I2C address!");
          }
  
          else {
            Serial.println("Device will not acknowledge on new I2C address.");
          }
        }
  
        else {
          Serial.println("Address out of range! Try an adress between 0x08 and 0x77");
        }
      }
  
      else {
        Serial.print("Invalid Text! Try again.");
      }
    }
    delay(100);
  }
}

void loop() {
  //if no I2C device found or Qwiic button correctly set to new address, 
  //scan for available I2C devices
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");

  delay(5000);           // wait 5 seconds for next scan
}
