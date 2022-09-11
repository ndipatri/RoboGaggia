/******************************************************************************
  Checks whether the button is pressed, and then prints its status over serial!

  Fischer Moseley @ SparkFun Electronics
  Original Creation Date: June 28, 2019

  This code is Lemonadeware; if you see me (or any other SparkFun employee) at the
  local, and you've found our code helpful, please buy us a round!

  Hardware Connections:
  Attach the Qwiic Shield to your Arduino/Photon/ESP32 or other
  Plug the button into the shield
  Print it to the serial monitor at 115200 baud.

  Distributed as-is; no warranty is given.
******************************************************************************/

//For this example to work, you must daisy chain together two Qwiic buttons with different addresses. 
//To change the address of a Qwiic button, please visit example 6

#include <SparkFun_Qwiic_Button.h>
QwiicButton button1;
QwiicButton button2;

void setup() {
  Serial.begin(115200);
  Serial.println("Qwiic button examples");
  Wire.begin(); //Join I2C bus

  //check if the buttons will acknowledge over I2C
  //connect to Qwiic button at address 0x5B
  if (button1.begin(0x5B) == false){
    Serial.println("Button 1 did not acknowledge! Freezing.");
    while(1);
  }
  //connect to Qwiic button at default address, 0x6F
  if (button2.begin() == false) {
    Serial.println("Button 2 did not acknowledge! Freezing.");
    while(1);
  }
  Serial.println("Both buttons acknowledged.");
}

void loop() {
  //check if button 1 is pressed, and tell us if it is!
  if (button1.isPressed() == true) {
    Serial.println("Button 1 is pressed!");
    while (button1.isPressed() == true)
      delay(10);  //wait for user to stop pressing
    Serial.println("Button 1 is not pressed.");
  }

  //check if button 2 is pressed, and tell us if it is!
  if (button2.isPressed() == true){
    Serial.println("Button 2 is pressed!");
    while (button2.isPressed() == true)
      delay(10);  //wait for user to stop pressing
    Serial.println("Button 2 is not pressed.");
  }
  delay(20); //Don't hammer too hard on the I2C bus.
}
