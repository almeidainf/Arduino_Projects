 /*
 LOYOLA UNIVERSITY CHICAGO
 COMP 450 - MICROPROGRAM/MICROPROCESS
 January 2015
 
 Tiago de Almeida - tdealmeida@luc.edu
 1394611
 
 A1: Your Computer Hello Arduino
 
 This program reads analog input from a light sensor and controls an LED accordingly.
 The more light the sensor can capture, the less the LED will glow.
 The less light the sensor can capture, the more the LED will glow.
 
 The Circuit:
 * Light sensor connected to A1
 * LED connect to 11
 
 */

#define sensorPin A1  // Input pin for the light sensor
#define ledPin 11     // Output pin for the LED

int sensorValue = 0;  // Variable to store the value coming from the sensor

void setup() {
  // Set up serial output for debugging
  Serial.begin(9600);
  Serial.println("Starting the program");
}

void loop() {
  // Read the value from the sensor
  //    The input goes from 0 to 1023.
  //    Since the LED accepts analog values from 0 to 255, we divide the input by 4.
  //    Also, we want the LED to glow more when there is less light being captured. Therefore, we subtract input/4 from 255.
  sensorValue = 255 - analogRead(sensorPin)/4;
  
  // Print out the read and calculated value
  Serial.println(sensorValue);
  
  // Set the value to the LED pin
  analogWrite(ledPin, sensorValue);

  // Wait a little bit
  delay(60);                  
}
