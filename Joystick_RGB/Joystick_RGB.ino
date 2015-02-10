 /*
 LOYOLA UNIVERSITY CHICAGO
 COMP 450 - MICROPROGRAM/MICROPROCESS
 Spring 2015
 
 Tiago de Almeida - tdealmeida@luc.edu
 1394611
 
 A2: Device Demo and Report
 
 This program reads input from a Joystick and uses the data to control the colors in an RGB LED light.
 Input is read from axis X and Y. Since the input is analog we can control the intensity of each color.
 As a result, it is possible to mix the three basic colors in different intensities and produce several resulting colors.
 
 */
// Defining pins
#define BLUE 9
#define GREEN 10
#define RED 11
#define X A0
#define Y A1

// Defining debounce length
#define DEBOUNCE 100

// Variables
int Rint, Gint, Bint;
int Xvalue, Yvalue;
int times;

// Setting up serial output
void setup() {
  Serial.begin(9600);
  Serial.println("Starting...");
}

void loop() {

  // Initializing values
  Rint = 0;
  Bint = 0;
  Gint = 0;
  times = DEBOUNCE;

// Read input DEBOUNCE times
  while(times != 0){
    Rint += (1023 - analogRead(X));
    Gint += analogRead(X);
    Bint += analogRead(Y);
    times--;
  }
  
// Calculate average of input values
  Rint = Rint / DEBOUNCE;
  Gint = Gint / DEBOUNCE;
  Bint = Bint / DEBOUNCE;
  
// Convert value to range 0..255
  Rint = Rint / 4;
  Gint = Gint / 4;
  Bint = Bint / 4;

// Printing out calculated values
  Serial.print("R: ");
  Serial.print(Rint);
  Serial.print("G: ");
  Serial.print(Gint);
  Serial.print("B: ");
  Serial.println(Bint);

// Writing calculated values
  analogWrite(RED, Rint);
  analogWrite(GREEN, Gint);
  analogWrite(BLUE, Bint);          
}
