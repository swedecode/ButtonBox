// Uses Ben Buxton's rotary encoder work!: https://github.com/buxtronix/arduino/tree/master/libraries/Rotary

// Grab all libraries that we need.
#include <Keypad.h>
#include <Joystick.h>
#include <Wire.h>
#include <Adafruit_MCP23017.h>

// Allows us to use 'mcp' for calls to the MCP23017 we are using.
Adafruit_MCP23017 mcp;

// Some definitions: We're using 7 rotaries (with buttons), and 12 stand-alone buttons.
#define ENABLE_PULLUPS
#define NUMROTARIES 7
#define NUMBUTTONS 19
// We have a button matrix to support the 24 buttons: 5 rows, 5 columns
#define NUMROWS 3
#define NUMCOLS 4

// This is a matrix that maps our buttons out for use in the keypad later.
byte buttons[NUMROWS][NUMCOLS] = {
  {0,1,2,3}
  {4,5,6,7},
  {8,9,10,11}
};

// we have two pins + gnd for each rotary encoder. 
// 1. CW rotation is one button (cwFn)
// 2. CCW rotation is another button (ccwFn), and 
// 3. the button press itself is a third (btnFn) for each encoder.
struct rotariesdef {
  byte pin1;
  byte pin2;
  int ccwFn;
  int cwFn;
  volatile unsigned char state;
};

// These are mcp pins, since the rotary encoders are all on the MCP.
// Each row represents one rotary encoder
rotariesdef rotaries[NUMROTARIES] {
  {3,4,12,13},
  {5,6,14,15},
  {7,8,16,17},
  {9,10,18,19},
  {11,12,20,21}
  {13,14,22,23}
  {15,16,24,25}
};

// We need to be able to hold the previous state.
int rotaryBtnState = 0;

struct rotaryButtonsdef {
  byte btnPin;
  int btnFn;
  int prevBtnState;
}

rotaryButtonsdef rotaryButtons[NUMROTARIES] {
  {1,26,0}; 
  {4,27,0}; 
  {5,28,0}; 
  {6,29,0}; 
  {7,30,0}; 
  {8,31,0}; 
  {9,32,0}; 
}


#define DIR_CCW 0x10
#define DIR_CW 0x20
#define R_START 0x0

#define R_CW_FINAL 0x1
#define R_CW_BEGIN 0x2
#define R_CW_NEXT 0x3
#define R_CCW_BEGIN 0x4
#define R_CCW_FINAL 0x5
#define R_CCW_NEXT 0x6

const unsigned char ttable[7][4] = {
  // R_START
  {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
  // R_CW_FINAL
  {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},
  // R_CW_BEGIN
  {R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},
  // R_CW_NEXT
  {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
  // R_CCW_BEGIN
  {R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},
  // R_CCW_FINAL
  {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW},
  // R_CCW_NEXT
  {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};

// The pins that are used for the rows
byte rowPins[NUMROWS] = {14,15,16};

// The pins that are used for the columns
byte colPins[NUMCOLS] = {18,19,21,21}; 

Keypad buttbx = Keypad( makeKeymap(buttons), rowPins, colPins, NUMROWS, NUMCOLS); 

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, 
  JOYSTICK_TYPE_GAMEPAD, 39, 0,
  false, false, false, false, false, false,
  false, false, false, false, false);

String msg;

void setup() {
  // Initiate the mcp and joystick packages, and run rotary_init().
  mcp.begin();
  Joystick.begin();
  rotary_init();
}

void loop() { 

  CheckAllEncoders();

  CheckAllButtons();
  
  CheckSingleButtons();

}

void CheckSingleButtons(void) {
  for (int i=0; i<NUMROTARIES; i++){
    int btnState
    btnState = digitalRead(RotaryButtons[i]) 
      
    if btnState != prevBtnState
  }
}


void CheckAllButtons(void) {
      if (buttbx.getKeys())
    {
       for (int i=0; i<LIST_MAX; i++)   
        {
           if ( buttbx.key[i].stateChanged )   
            {
            switch (buttbx.key[i].kstate) {  
                    case PRESSED:
                              msg = " pressed.";
                    case HOLD:
                              Joystick.setButton(buttbx.key[i].kchar, 1);
                              msg = " held.";
                              break;
                    case RELEASED:
                              msg = " released.";
                    case IDLE:
                              Joystick.setButton(buttbx.key[i].kchar, 0);
                              break;
            }
           }   
         }
     }
}

void rotary_init() {
  for (int i=0;i<NUMROTARIES;i++) {
    // We initialise all pins
    mcp.pinMode(rotaries[i].pin1, INPUT);
    mcp.pinMode(rotaries[i].pin2, INPUT);
    mcp.pinMode(rotaries[i].btnPin, INPUT);
    // And set them to be pullups
    mcp.digitalWrite(rotaries[i].pin1, HIGH);
    mcp.digitalWrite(rotaries[i].pin2, HIGH);
    mcp.digitalWrite(rotaries[i].btnPin, HIGH);
    }
}

unsigned char rotary_process(int _i) {
   unsigned char pinstate = (mcp.digitalRead(rotaries[_i].pin2) << 1) | mcp.digitalRead(rotaries[_i].pin1);
  rotaries[_i].state = ttable[rotaries[_i].state & 0xf][pinstate];
  return (rotaries[_i].state & 0x30);
}

void CheckAllEncoders(void) {
  for (int i=0;i<NUMROTARIES;i++) {
    unsigned char result = rotary_process(i);
    // If the result was that the rotary encoder turned counter-clockwise, we:
    // 1. set the ccwFn button to on (1)
    // 2. wait 50ms, and 
    // 3. and then turn it off (0).
    // If the rotation was clockwise, do the same thing for cwFn.
    
    if (result == DIR_CCW) {
      Joystick.setButton(rotaries[i].ccwFn, 1); delay(50); Joystick.setButton(rotaries[i].ccwFn, 0);
    };
    if (result == DIR_CW) {
      Joystick.setButton(rotaries[i].cwFn, 1); delay(50); Joystick.setButton(rotaries[i].cwFn, 0);
    } 
    }
  }
}
