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

#include <SparkFun_Qwiic_Button.h>
QwiicButton button;

void setup() {
  Serial.begin(115200);
  Serial.println("Qwiic button examples");
  Wire1.setClock(400000); //set I2C communication to 400kHz
  Wire1.begin(); //Compilation will fail here if your platform doesn't have multiple I2C ports

  //check if button will acknowledge over I2C
  if (button.begin(0x6F, Wire1) == false) {
    Serial.println("Device did not acknowledge! Freezing.");
    while(1);
  }
  Serial.println("Button acknowledged.");
}

void loop() {
  //check if button is pressed, and tell us if it is!
  if (button.isPressed() == true) {
    Serial.println("The button is pressed!");
    while (button.isPressed() == true)
      delay(10);  //wait for the user to stop pressing
    Serial.println("The button is not pressed.");
  }
  delay(20);  //Don't hammer too hard on the I2C bus
}
