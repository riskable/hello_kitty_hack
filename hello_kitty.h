// hello_kitty.h

// NOTE: When adding new modes make sure to increment the numModes variable
#define MODE_FADE 0
#define MODE_BLINK 1
#define MODE_STROBE 2
#define MODE_MANUAL 3
#define MODE_SIMON 4
#define LONG_PRESS_THRESHOLD 200
// These are absolute maximums (as opposed to an adjustable one)
#define MAXIMUM_RED 254
#define MAXIMUM_GREEN 130
#define MAXIMUM_BLUE 254
#define RED 0
#define GREEN 1
#define BLUE 2
// Used by the song playing function:
#define OCTAVE_OFFSET 0

// Pin utilization
const int ledPin = 13; // TEMPORARY FOR TESTING
// NOTE: Buzzer uses pins 9 and 10
const int redPin = 5;
const int greenPin = 6;
const int bluePin = 3;
const int touchPin1 = A1;
const int touchPin2 = A0;
const int touchPin3 = A2;
const int touchPin4 = 8;
const int modeButtonPin = 12;
// const int photoresistorPin = A0;

// Timer intervals... each associated function will get called this often (in milliseconds)
const int modeSwitchInterval = 250; // Brief period to indicate when modes are changed
const int fadeInterval = 40; // Milliseconds between calls to fadeLeds()
const int fadeStep = 1; // Increase this to fade faster (inside of fadeLeds())
const int blinkInterval = 500; // Milliseconds between calls to blinkLeds()
const int flashInterval = 30; // Milliseconds between flashes (strobe!)
const int lightingCheckInterval = 100; // How often to check the photoresistor
const int buttonCheckInterval = 10; // How often to check the state of the button
const int touchCheckInterval = 10; // How often to check the state of the touch sensors
const int simonInterval = 10; // How often to check for state changes in the Simon game

// Touch thresholds and values
const long touchThreshold = 3; // Above this value indicates a touch event
// So every function can know the state of the touch sensors:
int touch1held = 0;
int touch2held = 0;
int touch3held = 0;
int touch4held = 0;

// Buzzer thresholds and values
const int maxVolume = 7; // Goes up 10 but that's probably too loud
int volume = 1;
const int minBeepFreq = 3000;
const int maxBeepFreq = 15000;
int beepFrequency = minBeepFreq;

// RGB control and colors
struct RGB {
    byte r;
    byte g;
    byte b;
};
RGB Red = { MAXIMUM_RED, 0, 0 };
RGB Green = { 0, MAXIMUM_GREEN, 0 };
RGB Blue = { 0, 0, MAXIMUM_BLUE };
RGB Yellow = { MAXIMUM_RED, MAXIMUM_GREEN, 0 };
RGB White = { MAXIMUM_RED, MAXIMUM_GREEN, MAXIMUM_BLUE };
int maxBrightness = 254; // Up to 254 (which would probably overheat poor Kitty!)
int redBrightness = 0;
int greenBrightness = 0;
int blueBrightness = 0;
int colorPins[] = { bluePin, redPin, greenPin };
int colorLevels[] = { 0, 0, 0 }; // Blue, Red, Green
int currentColor = 0; // Start with blue (the colorLevels index)
int prevColor = 2;
int flashState = 0; // Used by strobeLeds() to track on/off states
long photoresistorVal = 0;
int manualLightingMode = 0; // For determining if manual lighting mode is enabled
int buttonHeldCount = 0; // So we can detect a long press of the button

// Operating modes:
//   0: Fading from one color to the next (like the original Hello Kitty).
//   1: Red, Green, Blue, repeat.
//   2: Flash all with speed controlled via the touch pads.
//   3: Manual mode...  Play with the LEDs/colors.
//   4: Play the classic game of simon.
const int numModes = 3; // NOTE: This is 0-based so 4 means "five modes"
int mode = 0; // Start with mode 0
int buttonState = 0;

// Simon game stuff
// int starttune[] = {NOTE_C4, NOTE_F4, NOTE_C4, NOTE_F4, NOTE_C4, NOTE_F4, NOTE_C4, NOTE_F4, NOTE_G4, NOTE_F4, NOTE_E4, NOTE_F4, NOTE_G4};
// int duration2[] = {100, 200, 100, 200, 100, 400, 100, 100, 100, 100, 200, 100, 500};
// int note[] = {NOTE_C4, NOTE_C4, NOTE_G4, NOTE_C5, NOTE_G4, NOTE_C5};
// int duration[] = {100, 100, 100, 300, 100, 300};
// boolean button[] = {2, 3, 4, 5}; //The four button input pins
int simonStarted = 0; // So we can tell if we've already started a game
int turn = 0;  // turn counter
int buttonstate = 0;  // button state checker
int randomArray[100]; //Intentionally long to store up to 100 inputs (doubtful anyone will get this far)
int inputArray[100];
const int SimonRedNote = NOTE_G3;
const int SimonGreenNote = NOTE_A3;
const int SimonBlueNote = NOTE_B3;
const int SimonYellowNote = NOTE_C4;

// Song stuff
// notes in the melody:
typedef struct {
    int note;
    int duration;
} Note;
Note melody[] = { {NOTE_C4, 4}, {NOTE_G3, 8}, {NOTE_G3, 8}, {NOTE_GS3, 8}, {NOTE_G3, 4}, {0, 4}, {NOTE_B3, 4}, {NOTE_C4, 4} };
// Note simonTune[] = { {NOTE_C4, 4} };
int songIndex = 0; // For async song playing
unsigned long lastNoteStart = 0;
Note* playingSong;


// int notes[] = {
//   0,
//   NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4,
//   NOTE_C5, NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5,
//   NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6, NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6,
//   NOTE_C7, NOTE_CS7, NOTE_D7, NOTE_DS7, NOTE_E7, NOTE_F7, NOTE_FS7, NOTE_G7, NOTE_GS7, NOTE_A7, NOTE_AS7, NOTE_B7
// };
// char *Pacman = "pacman:d=4,o=5,b=112:32b,32p,32b6,32p,32f#6,32p,32d#6,32p,32b6,32f#6,16p,16d#6,16p,32c6,32p,32c7,32p,32g6,32p,32e6,32p,32c7,32g6,16p,16e6,16p,32b,32p,32b6,32p,32f#6,32p,32d#6,32p,32b6,32f#6,16p,16d#6,16p,32d#6,32e6,32f6,32p,32f6,32f#6,32g6,32p,32g6,32g#6,32a6,32p,32b.6";
// char *wwtbam1 = "wwtbam1:d=4,o=5,b=140:16g,16c6,16d#6,16g6,16c7,16g6,16d#6,16c6,16g,16c6,16d#6,16g6,16c7,16g6,16d#6,16c6,16g#,16c6,16d#6,16g#6,16c7,16g#6,16d#6,16c6,16g#,16c6,16d#6,16g#6,16c7,16g#6,16d#6,16c6,16f#,16c6,16d#6,16f#6,16c7,16f#6,16d#6,16c6,16f#,16c6,16d#6,16f#6,16c7,16f#6,16d#6,16c6,16g,16b,16d6,16g6,16b6,16g6,16d6,16b,16g,16b,16d6,16g6,16b6,16g6,16d6,16b";
// char *BarbieGirl = "Barbie girl:d=4,o=5,b=125:8g#,8e,8g#,8c#6,a,p,8f#,8d#,8f#,8b,g#,8f#,8e,p,8e,8c#,f#,c#,p,8f#,8e,g#,f#";
// char *KnightRider = "KnightRider:d=4,o=5,b=63:16e,32f,32e,8b,16e6,32f6,32e6,8b,16e,32f,32e,16b,16e6,d6,8p,p,16e,32f,32e,8b,16e6,32f6,32e6,8b,16e,32f,32e,16b,16e6,f6,p";
// char *AxelF = "axelf:d=4,o=5,b=160:f#,8a.,8f#,16f#,8a#,8f#,8e,f#,8c.6,8f#,16f#,8d6,8c#6,8a,8f#,8c#6,8f#6,16f#,8e,16e,8c#,8g#,f#";

// Function definitions
void nextMode(void);
void blinkLeds(void);
void nextColor(void);
void fadeLeds(void);
void strobeLeds(void);
void whiteNoise(void);
void beep(void);
void beep2(void);
void allOff(void);
void lightingCheck(void);
void touchCheck(void);
void buttonCheck(void);
void simonStartup(void);
void simonGame(void);
void playSong(Note* song);
void play(Note* song);
void playBackground(void);
uint8_t readCapacitivePin(int pinToMeasure);
// void play_rtttl(char *p);
void playSimon(void);
void fail(void);
void simonInput(void);
