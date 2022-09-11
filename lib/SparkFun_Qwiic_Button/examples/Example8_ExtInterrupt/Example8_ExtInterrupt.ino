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
uint8_t brightness = 100;   //The brightness to set the LED to when interrupt pin is high
                            //Can be any value between 0 (off) and 255 (max)
int interruptPin = 2; //pin that will change states when interrupt is triggered

void setup() {
  Serial.begin(115200);
  Serial.println("Qwiic button examples");

  Wire.begin(); //Join I2C bus

  //intialize interrupt pin
  pinMode(interruptPin, INPUT_PULLUP);

  //check if button will acknowledge over I2C
  if (button.begin() == false) {
    Serial.println("Device did not acknowledge! Freezing.");
    while (1);
  }
  Serial.println("Button acknowledged.");

  button.enablePressedInterrupt();  //configure the interrupt pin to go low when we press the button.
  button.enableClickedInterrupt();  //configure the interrupt pin to go low when we click the button.
  button.clearEventBits();  //once event bits are cleared, interrupt pin goes high
}

void loop() {
  //check state of interrupt pin
  //interrupt pin goes low when user presses the button
  if (digitalRead(interruptPin) == LOW) {
    Serial.println("Button event!");

    //check if button is pressed, and tell us if it is!
    if (button.isPressed() == true) {
      Serial.println("The button is pressed!");
    }

    //check if the button has been clicked, and tell us if it is!
    if (button.hasBeenClicked() == true) {
      Serial.println("The button has been clicked!");
    }
    button.clearEventBits();  //once event bits are cleared, interrupt pin goes high
  }
  delay(20); //Don't hammer too hard on the I2C bus
}
