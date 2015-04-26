// Timer Library (http://playground.arduino.cc/Code/Timer)
#include <Event.h>
#include <Timer.h>

#define PIN_SCK  13
#define PIN_MISO 12
#define	PIN_MOSI 11
#define PIN_SS   10

#define BUTTON_PIN 8
#define SIZE 64

#define GREEN 24
#define YELLOW 36
#define BLUE 11
#define RED 32

#define WALL_COLOR GREEN
#define BIRD_COLOR RED
#define FIRST_WALL_POS 7

#define TENS 1
#define UNITS 4

#define DOWN -1
#define STRAIGHT 0
#define UP 1

typedef enum GameState{
  STARTED,
  STOPPED,
} GameState;

typedef struct Wall{
  byte bricks;
  byte xpos;
} Wall;

typedef struct Game{
  GameState state;
  int score;
  float vy; // y velocity
  float birdY; // y position between 0 and 1
  Wall wallOne;
  Wall wallTwo;
} Game;

const float kG = 0.005; // gravity
const float kLift = -0.05; // button lift
const byte kXpos = 1; // the bird is stuck on col 1 & 2

Game gGame; // the global game
Timer gTimer; // the global time

// time events
int gUpdateEvent;
int gMoveWallOneEvent;
int gMoveWallTwoEvent;

// LED Grid
char frame[SIZE];

// Send a single byte via SPI, taken from Atmel's datasheet example.
void sendChar(char cData) {
  SPDR = cData;
  while (!(SPSR & (1 << SPIF)));
}

// Send a full frame to the LED matrix
void sendFrame(){
  // Assert SS
  digitalWrite(PIN_SS, LOW);
  // delay as the LED Matrix datasheet's recommends
  delayMicroseconds(500);
  // send the full buffer
  for (int i = 0; i < SIZE; i++) {
    char c;
    c = *(frame + i);
    // This is needed because sending a '%' will reconfigure the
    // board for daisy chain operation
    if ('%' == c)
      sendChar((2 << 5) & (1 << 2) & 1); // similar color
    else
      sendChar(c);
  }
  // de-assert SS
  digitalWrite(PIN_SS, HIGH);
  // The LED Matrix datasheet's recommends this delay on daisy
  // chain configurations
  //delayMicroseconds(10);
  delay(5);
}
 
// Turn entire grid off
void resetGrid(){
  for(int i = 0; i < SIZE; i++)
    frame[i] = 0;
  sendFrame();
}

// Set entire grid on with chosen color
void setGrid(int color){
  for(int i = 0; i < SIZE; i++)
    frame[i] = color;
  sendFrame();
}

// Turn a given led on, with specified color
void setLED(int x, int y, int color){
  frame[(y)*8 + x] = color;
}

// Turn a given led off
void unsetLED(int x, int y){
  frame[(y)*8 + x] = 0;
}

void initialScreen(){
  // frame
  for(int j = 0; j < 8; j+=7)
    for(int i = 0; i < 8; i++)
      setLED(i, j, GREEN);
  for(int j = 0; j < 8; j+=7)
    for(int i = 1; i < 7; i++)
      setLED(j, i, GREEN);
  
  // letter P
  for(int i = 1; i < 7; i++)
    setLED(2, i, YELLOW);
  setLED(3, 6, YELLOW);
  setLED(4, 6, YELLOW);
  setLED(5, 6, YELLOW);
  setLED(3, 4, YELLOW);
  setLED(4, 4, YELLOW);
  setLED(5, 4, YELLOW);
  setLED(5, 5, YELLOW);
  
  sendFrame();
}

void setup(){
// LED Grid
  SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);
  SPSR = SPSR & B11111110;

  pinMode(PIN_SCK, OUTPUT);
  digitalWrite(PIN_SCK, LOW); // ensure clock low
  pinMode(PIN_MOSI, OUTPUT);
  pinMode(PIN_SS, OUTPUT);
  digitalWrite(PIN_SS, HIGH); // de-assert SS
  delayMicroseconds(500); // delay as the LED Matrix datasheet's recommends

  digitalWrite(PIN_SS, LOW);
  delayMicroseconds(500);
  sendChar('%');
  sendChar(1);
  digitalWrite(PIN_SS, HIGH);
  delayMicroseconds(10);

// Button
  pinMode(BUTTON_PIN, INPUT);

// Game
  randomSeed(analogRead(0));
  gGame.state = STOPPED;
  gTimer.every(30, reactToUserInput);
  initialScreen();
  
// Serial output
  Serial.begin(9600);
  Serial.println("Starting...");
}

void startGame(boolean doit){
  if(doit){
    gGame.score = 0;
    gGame.state = STARTED;
    gGame.birdY = 0.5;
    gGame.wallOne.xpos = FIRST_WALL_POS;
    gGame.wallOne.bricks = generateWall();
    gGame.wallTwo.xpos = FIRST_WALL_POS;
    gGame.wallTwo.bricks = generateWall();

    gUpdateEvent = gTimer.every(50, updateBirdPosition);
    gTimer.after(2500, startWallOne);
    gTimer.after(3300, startWallTwo);
    resetGrid();
  }
  else{
    gGame.state = STOPPED;
    gTimer.stop(gUpdateEvent);
    gTimer.stop(gMoveWallOneEvent);
    gTimer.stop(gMoveWallTwoEvent);
  }
}

void drawWall(struct Wall *wall, byte x){
  for (byte row = 0; row < 8; row++)
    if (wall->bricks & (0x80 >> row))
      setLED(x, row, WALL_COLOR);
}

void eraseWall(struct Wall *wall, byte x){
  for (byte row = 0; row < 8; row++)
    if (wall->bricks & (0x80 >> row))
      unsetLED(x, row);
}

void moveWall(Wall *wall){
  if (wall->xpos == 255) { // wall has come past screen
    eraseWall(wall, 0);
    wall->bricks = generateWall();
    wall->xpos = FIRST_WALL_POS;
  }
  else if (wall->xpos < FIRST_WALL_POS)
    eraseWall(wall, wall->xpos + 1);

  drawWall(wall, wall->xpos);

  // check if the wall just slammed into the bird.
  if (wall->xpos == 2) {
    byte ypos = 7 * gGame.birdY;  
    if (wall->bricks & (0x80 >> ypos)) {explode(); gameOver();}
    else gGame.score++; // no collision: score!
  }
  wall->xpos = wall->xpos - 1;
}

void startWallOne() {gMoveWallOneEvent = gTimer.every(200, moveWallOne);}
void startWallTwo() {gMoveWallTwoEvent = gTimer.every(200, moveWallTwo);}
void moveWallOne() {moveWall(&gGame.wallOne);}
void moveWallTwo() {moveWall(&gGame.wallTwo);}

byte generateWall(){
  byte gap = random(3, 6); // size of the hole in the wall
  byte punch = (1 << gap) - 1; // the hole expressed as bits
  byte slide = random(1, 8 - gap); // the hole's offset
  return 0xff & ~(punch << slide); // the wall without the hole
}

void reactToUserInput(){
  static int old = 0;
  int buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == HIGH){
    if (gGame.state == STARTED){
      if (!old){ // button was not pressed last time we checked
        if (gGame.vy > 0)
          gGame.vy = kLift; // initial bounce
        else
          gGame.vy += kLift; // keep adding lift
      }
    }
    else // game is not playing. start it.
      startGame(true); 
  }
  
  old = buttonState;
}

void drawBird(int direction, byte yHead){
  // previous position of tail and head (one pixel each)
  static byte cTail, cHead;
  byte yTail;
  yTail = constrain(yHead - direction, 0, 7);

  // erase it from old position
  unsetLED(kXpos, cTail);
  unsetLED(kXpos+1, cHead);

  // draw it in new position
  setLED(kXpos, yTail, BIRD_COLOR);
  setLED(kXpos+1, yHead, BIRD_COLOR);

  // remember current position 
  cTail = yTail;
  cHead = yHead;
}

void updateBirdPosition(){
  // initial position (simulated screen size 0..1)
  gGame.vy += kG; // apply gravity
  float oldY = gGame.birdY; // store old position
  gGame.birdY -= gGame.vy; // calculate new y position // ###

  if (gGame.birdY > 1){ // peg y to top or bottom
    gGame.birdY = 1;
    gGame.vy = 0;
  }
  else if (gGame.birdY < 0){
    gGame.birdY = 0;
    gGame.vy = 0;
  }

  // convert to screen position
  byte ypos = 7 * gGame.birdY;
  // define direction
  int direction;
  if (abs(oldY - gGame.birdY) < 0.01) direction = STRAIGHT;
  else if (oldY < gGame.birdY) direction = UP;
  else direction = DOWN;
  
  drawBird(direction, ypos);
}

void explode(){
  for(int i = 0; i < 20; i++){
    setGrid(RED);
    delay(25);
    setGrid(BLUE);
    delay(25);
  }
  resetGrid();
  delay(200);
}

void gameOver(){
  setScore(gGame.score);
  startGame(false);
}

void loop(){
  gTimer.update();
  sendFrame();
}

void setScore(int score){
  
  Serial.print("Score: ");
  Serial.println(gGame.score);
  
  int hundreds = 0, tens = 0, units;
  
  units = score;
  while(units > 9){
    units -= 10;
    tens++;
  }
  
  while(tens > 9){
    tens -= 10;
    hundreds++;
  }

  for(int i = 0; i < hundreds; i++)
    setLED(i, 0, BLUE);

  setNumber(TENS, tens);
  setNumber(UNITS, units);
  sendFrame();
  delay(200);
}

void setNumber(int pos, int number){
  
  switch(number){
    case 0:
      setLED(pos, 6, BLUE);
      setLED(pos+1, 6, BLUE);
      setLED(pos+2, 6, BLUE);
      setLED(pos, 5, BLUE);
      setLED(pos, 4, BLUE);
      setLED(pos, 3, BLUE);
      setLED(pos+2, 5, BLUE);
      setLED(pos+2, 4, BLUE);
      setLED(pos+2, 3, BLUE);
      setLED(pos, 2, BLUE);
      setLED(pos+1, 2, BLUE);
      setLED(pos+2, 2, BLUE);
      break;
  
  case 1:
      setLED(pos+1, 6, BLUE);
      setLED(pos+1, 5, BLUE);
      setLED(pos+1, 4, BLUE);
      setLED(pos+1, 3, BLUE);
      setLED(pos+1, 2, BLUE);
      break;
      
  case 2:
      setLED(pos, 6, BLUE);
      setLED(pos+1, 6, BLUE);
      setLED(pos+2, 6, BLUE);
      setLED(pos, 4, BLUE);
      setLED(pos+1, 4, BLUE);
      setLED(pos+2, 4, BLUE);
      setLED(pos, 2, BLUE);
      setLED(pos+1, 2, BLUE);
      setLED(pos+2, 2, BLUE);
      setLED(pos+2, 5, BLUE);
      setLED(pos, 3, BLUE);
      break;
      
  case 3:
      setLED(pos, 6, BLUE);
      setLED(pos+1, 6, BLUE);
      setLED(pos+2, 6, BLUE);
      setLED(pos, 4, BLUE);
      setLED(pos+1, 4, BLUE);
      setLED(pos+2, 4, BLUE);
      setLED(pos, 2, BLUE);
      setLED(pos+1, 2, BLUE);
      setLED(pos+2, 2, BLUE);
      setLED(pos+2, 5, BLUE);
      setLED(pos+2, 3, BLUE);
      break;

  case 4:
      setLED(pos+2, 6, BLUE);
      setLED(pos+2, 5, BLUE);
      setLED(pos+2, 4, BLUE);
      setLED(pos+2, 3, BLUE);
      setLED(pos+2, 2, BLUE);
      setLED(pos, 6, BLUE);
      setLED(pos, 5, BLUE);
      setLED(pos, 4, BLUE);
      setLED(pos+1, 4, BLUE);
      break;

   case 5:
      setLED(pos, 6, BLUE);
      setLED(pos+1, 6, BLUE);
      setLED(pos+2, 6, BLUE);
      setLED(pos, 4, BLUE);
      setLED(pos+1, 4, BLUE);
      setLED(pos+2, 4, BLUE);
      setLED(pos, 2, BLUE);
      setLED(pos+1, 2, BLUE);
      setLED(pos+2, 2, BLUE);
      setLED(pos, 5, BLUE);
      setLED(pos+2, 3, BLUE);
      break;
      
  case 6:
      setLED(pos, 6, BLUE);
      setLED(pos+1, 6, BLUE);
      setLED(pos+2, 6, BLUE);
      setLED(pos, 4, BLUE);
      setLED(pos+1, 4, BLUE);
      setLED(pos+2, 4, BLUE);
      setLED(pos, 2, BLUE);
      setLED(pos+1, 2, BLUE);
      setLED(pos+2, 2, BLUE);
      setLED(pos, 5, BLUE);
      setLED(pos+2, 3, BLUE);
      setLED(pos, 3, BLUE);
      break;

  case 7:
      setLED(pos, 6, BLUE);
      setLED(pos+1, 6, BLUE);
      setLED(pos+2, 6, BLUE);
      setLED(pos+2, 5, BLUE);
      setLED(pos+2, 4, BLUE);
      setLED(pos+2, 3, BLUE);
      setLED(pos+2, 2, BLUE);
      break;
      
  case 8:
      setLED(pos, 6, BLUE);
      setLED(pos+1, 6, BLUE);
      setLED(pos+2, 6, BLUE);
      setLED(pos, 4, BLUE);
      setLED(pos+1, 4, BLUE);
      setLED(pos+2, 4, BLUE);
      setLED(pos, 2, BLUE);
      setLED(pos+1, 2, BLUE);
      setLED(pos+2, 2, BLUE);
      setLED(pos+2, 5, BLUE);
      setLED(pos, 5, BLUE);
      setLED(pos+2, 3, BLUE);
      setLED(pos, 3, BLUE);
      break;
      
  case 9:
      setLED(pos, 6, BLUE);
      setLED(pos+1, 6, BLUE);
      setLED(pos+2, 6, BLUE);
      setLED(pos, 4, BLUE);
      setLED(pos+1, 4, BLUE);
      setLED(pos+2, 4, BLUE);
      setLED(pos, 2, BLUE);
      setLED(pos+1, 2, BLUE);
      setLED(pos+2, 2, BLUE);
      setLED(pos+2, 5, BLUE);
      setLED(pos, 5, BLUE);
      setLED(pos+2, 3, BLUE);
      break;
  }
}
