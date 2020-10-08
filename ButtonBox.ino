// Uses Ben Buxton's rotary encoder work!: https://github.com/buxtronix/arduino/tree/master/libraries/Rotary

// Grab all libraries that we need.
#include <Keypad.h>
#include <Joystick.h>
#include <Wire.h>
#include <Adafruit_MCP23017.h>

// Allows us to use 'mcp' for calls to the MCP23017 we are using.
Adafruit_MCP23017 mcp;

// Some definitions: We're using 5 rotaries (with buttons), and 24 stand-alone buttons.
#define ENABLE_PULLUPS
#define NUMROTARIES 5
#define NUMBUTTONS 24
// We have a button matrix to support the 24 buttons: 5 rows, 5 columns
#define NUMROWS 5
#define NUMCOLS 5

// This is a matrix that maps our buttons out for use in the keypad later.
byte buttons[NUMROWS][NUMCOLS] = {
  {0,1,2,3,4},
  {5,6,7,8,9},
  {10,11,12,13,14},
  {15,16,17,18,19},
  {20,21,22,23}
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
  int btnPin;
  int btnFn;
  int btnState;
};

// These are mcp pins, since the rotary encoders are all on the MCP.
// Each row represents one rotary encoder
rotariesdef rotaries[NUMROTARIES] {
  {7,6,29,30,0,9,24,0},
  {5,4,31,32,0,10,25,0},
  {3,2,33,34,0,11,26,0},
  {1,0,35,36,0,12,27,0},
  {15,14,37,38,0,13,28,0}
};

// We need to be able to hold the previous state.
int rotaryBtnState = 0;

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
byte rowPins[NUMROWS] = {4,5,6,8,9};

// The pins that are used for the columns
byte colPins[NUMCOLS] = {10,14,15,16,18}; 

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

    // If the encoder did not turn, we should check if the rotary button was pressed.
    else {
      rotaryBtnState = mcp.digitalRead(rotaries[i].btnPin);
      // If the button has been pressed, the rotaryBtnState will be LOW. 
      while (rotaryBtnState == LOW) {
        // If the rotaries[i] value is 0, it means that it's a new press, so we need to set it to on (1).
        if (rotaries[i].btnState == 0) {
          Joystick.setButton(rotaries[i].btnFn, 1);
          rotaries[i].btnState = 1;
        }
        // Now we read the button state again, which allows the button to be continuously checked for being pressed.
        // This means that it will only read this encoder button and nothing else meanwhile it is pressed, though!
        rotaryBtnState = mcp.digitalRead(rotaries[i].btnPin);
        
      }
      // If the button has been released, and the btnState value was set to 1, 
      // it means it was released and we need to do something: turn it off (set to 0).
      if (rotaryBtnState == HIGH && rotaries[i].btnState == 1) {
          Joystick.setButton(rotaries[i].btnChar, 0);
          rotaries[i].btnState = 0;
      }
      
    }
    
    
  }
}
