/******************************************************************************
  Checks whether the button is pressed, and light it up if it is! Also prints
  status to the serial monitor.

  Fischer Moseley @ SparkFun Electronics
  Original Creation Date: July 24, 2019

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
//Define LED characteristics
uint8_t brightness = 250;   //The maximum brightness of the pulsing LED. Can be between 0 (min) and 255 (max)
uint16_t cycleTime = 1000;   //The total time for the pulse to take. Set to a bigger number for a slower pulse, or a smaller number for a faster pulse
uint16_t offTime = 200;     //The total time to stay off between pulses. Set to 0 to be pulsing continuously.

void setup() {
  Serial.begin(115200);
  Serial.println("Qwiic button examples");
  Wire.begin(); //Join I2C bus

  //check if button will acknowledge over I2C
  if (button.begin() == false) {
    Serial.println("Device did not acknowledge! Freezing.");
    while (1);
  }
  Serial.println("Button acknowledged.");
  button.LEDoff();  //start with the LED off
}

void loop() {
  //check if button is pressed, and tell us if it is!
  if (button.isPressed() == true) {
    Serial.println("The button is pressed!");
    button.LEDconfig(brightness, cycleTime, offTime);
    while (button.isPressed() == true)
      delay(10);  //wait for user to stop pressing
    Serial.println("The button is not pressed.");
    button.LEDoff();
  }
  delay(20); //let's not hammer too hard on the I2C bus
}
