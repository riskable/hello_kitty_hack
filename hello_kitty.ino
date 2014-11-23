/*

Hello Kitty pillow light upgrade!  Instead of being a faint/dim three-color
fading blue/green/yellow LED combo it now has the following:

    * 3x 12V 5050 super bright RGB LEDs!
    * 4x capacitive touch pads (copper tape).
    * A speaker (a buzzer would work too).

REMOVED:

    - A photoresistor (to know when it's dark).  Couldn't find a good place for
      it on Kitty.

BUILD NOTES:

    If you're going to try this make sure that the wires for your capacitive
    touch sensors are as far away as possible from your transistors/MOSFETs
    powering your LEDs (if any).  They were showing max values (as if being
    touched) until I switched which side of the Arduino they were connected.

*/

// Includes
#include "pitches.h" // For buzzer
#include <toneAC.h> // For buzzer
#include "SimpleTimer.h"
#include "hello_kitty.h"

// Asynchronous code stuff
SimpleTimer timer;
int lightingTimer;
int fadeTimer;
int blinkTimer;
int strobeTimer;
int simonTimer;
int backgroundTimer;

void setup()
{
//     Serial.begin(9600);
    // Configure pins
    pinMode(ledPin, OUTPUT);
    pinMode(redPin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(bluePin, OUTPUT);
    pinMode(modeButtonPin, INPUT_PULLUP);
    randomSeed(analogRead(0)); // Add more/better entropy to the random number generator
//     lightingTimer = timer.setInterval(lightingCheckInterval, lightingCheck);
    fadeTimer = timer.setInterval(fadeInterval, fadeLeds);
    blinkTimer = timer.setInterval(blinkInterval, blinkLeds);
    strobeTimer = timer.setInterval(flashInterval, strobeLeds);
    simonTimer = timer.setInterval(simonInterval, simonGame);
    // These timers will never need to be cancelled or modified so there's no need to assign them to globals:
    timer.setInterval(buttonCheckInterval, buttonCheck);
    timer.setInterval(touchCheckInterval, touchCheck);
    backgroundTimer = timer.setInterval(1, playBackground);
    timer.disable(backgroundTimer);
}

// All this code runs (mostly) asynchronously via the SimpleTimer lib.
void loop() {
    timer.run();
}

void beep(void) {
    // Beeps once then calls a second, lower beep a moment later
    noToneAC(); // Turn off the buzzer in case it's on
    toneAC(beepFrequency, volume, 100);
// Disabled this because the secondary beep sounded strange under certain conditions...
//     timer.setTimeout(250, beep2);
}

void beep2(void) {
    // Beeps once then increments the beepFrequency (resetting it to 3000 if it goes over 15000)
    // Turn off the buzzer in case it's on
    toneAC(beepFrequency-2000, volume, 100);
    beepFrequency += 1000;
    if (beepFrequency > 15000) {
        beepFrequency = minBeepFreq;
    }
}

void displayColor(RGB color) {
    if (color.r > maxBrightness) {
        color.r = maxBrightness;
    }
    if (color.g > maxBrightness) {
        color.g = maxBrightness;
    }
    if (color.b > maxBrightness) {
        color.b = maxBrightness;
    }
    analogWrite(redPin, color.r);
    analogWrite(greenPin, color.g);
    analogWrite(bluePin, color.b);
}

void allOff(void) {
    // Turns off all LEDs and resets all color states to 0
    digitalWrite(redPin, LOW);
    digitalWrite(greenPin, LOW);
    digitalWrite(bluePin, LOW);
    // Restore the default brightness for each LED
    colorLevels[0] = 0;
    colorLevels[1] = 0;
    colorLevels[2] = 0;
}

void nextMode(void) {
    beep();
    mode += 1;
    if (mode > numModes) {
        mode = 0; // Back to the start
    }
    if (mode != MODE_FADE) {
        timer.disable(fadeTimer);
    } else {
        timer.enable(fadeTimer);
    }
    if (mode != MODE_BLINK) {
        timer.disable(blinkTimer);
    } else {
        timer.enable(blinkTimer);
    }
    if (mode != MODE_STROBE) {
        timer.disable(strobeTimer);
    } else {
        timer.enable(strobeTimer);
    }
    if (mode != MODE_SIMON) {
        timer.disable(simonTimer);
        simonStarted = 0;
    } else {
        timer.enable(simonTimer);
    }
    allOff();
}

void blinkLeds(void) {
    // Blink each LED color in sequence
    if (mode != MODE_BLINK) { // Only do this if we're in blink mode
        return;
    }
    if (maxBrightness == 0) {
        allOff();
        return;
    }
    int currentColorPin = colorPins[currentColor];
    allOff();
    analogWrite(currentColorPin, maxBrightness);
    nextColor();
}

void nextColor(void) {
    // Used by blinkLeds() and fadeLeds() to iterate colors
    prevColor = currentColor;
    currentColor += 1;
    if (currentColor > 2) {
        currentColor = 0;
    }
}

void fadeLeds(void) {
    // Fade from one color to the next.
    // Meant to be called at a regular interval.
    if (mode != MODE_FADE) { // Only do this if we're in fade mode
        return;
    }
    if (maxBrightness == 0) {
        allOff();
        return;
    }
    int currentColorPin = colorPins[currentColor];
    int prevColorPin = colorPins[prevColor];
    colorLevels[currentColor] += fadeStep;
    if (colorLevels[prevColor] > 0) {
        colorLevels[prevColor] -= fadeStep;
    }
    analogWrite(currentColorPin, colorLevels[currentColor]);
    analogWrite(prevColorPin, colorLevels[prevColor]);
    if (colorLevels[currentColor] >= maxBrightness) {
        nextColor();
    }
}

void strobeLeds(void) {
    // Blink all LEDs at once (strobe)
    if (mode != MODE_STROBE) { // Only do this if we're in fade mode
        return;
    }
    if (maxBrightness == 0) {
        allOff();
        return;
    }
    if (flashState == 0) {
        flashState = maxBrightness;
    } else {
        flashState = 0;
    }
    if (touch1held == 0) {
        if (touch2held == 0) {
            analogWrite(redPin, flashState);
        } else {
            analogWrite(redPin, 0);
        }
        if (touch3held == 0) {
            analogWrite(greenPin, flashState);
        } else {
            analogWrite(greenPin, 0);
        }
        if (touch4held == 0) {
            analogWrite(bluePin, flashState);
        } else {
            analogWrite(bluePin, 0);
        }
    }
}

// Disabled since I haven't found a good place to put the sensor
// void lightingCheck(void) {
//     // Sets the *light* variable to 1 if the lights are on; to 0 if not
//     if (manualLightingMode == 1) {
//         maxBrightness = MAXIMUM_BRIGHTNESS;
//         return;
//     }
//     photoresistorVal = analogRead(photoresistorPin);
// //     Serial.println(photoresistorVal);
//     if (photoresistorVal > 50) {
//         maxBrightness = 0;
//         allOff();
//     } else {
//         maxBrightness = MAXIMUM_BRIGHTNESS;
//     }
// }

void buttonCheck(void) {
    // Checks the state of the mode button and takes action if warranted
    // NOTE: The default button check interval is 10ms
    buttonState = digitalRead(modeButtonPin);
    Serial.print("buttonState: ");
    Serial.print(buttonState);
    // if it is, the buttonState is LOW:
    if (buttonState == LOW) {
        buttonHeldCount += 1;
        if (buttonHeldCount > LONG_PRESS_THRESHOLD) {
            buttonHeldCount = 0; // Reset so we don't change modes
            noToneAC();
            toneAC(NOTE_C6, volume, 100);
            delay(100);
            toneAC(NOTE_C6, volume, 100);
            delay(100);
            toneAC(NOTE_F6, volume, 100);
            if (manualLightingMode == 0) {
                manualLightingMode = 1;
            } else {
                manualLightingMode = 0;
            }
        }
    } else {
        if (buttonHeldCount > 0) {
            nextMode();
            buttonHeldCount = 0;
        }
    }
}

void touchCheck(void) {
    // Checks the state of all touch sensors and performs our touch-specific actions
    uint8_t touch1val = readCapacitivePin(touchPin1);
    uint8_t touch2val = readCapacitivePin(touchPin2);
    uint8_t touch3val = readCapacitivePin(touchPin3);
    uint8_t touch4val = readCapacitivePin(touchPin4);

    if (touch1val > touchThreshold) {
        touch1held = 1;
    } else {
        touch1held = 0;
    }
    if (touch2val > touchThreshold) {
        touch2held = 1;
    } else {
        touch2held = 0;
    }
    if (touch3val > touchThreshold) {
        touch3held = 1;
    } else {
        touch3held = 0;
    }
    if (touch4val > touchThreshold) {
        touch4held = 1;
    } else {
        touch4held = 0;
    }

    switch (mode) {
        case MODE_FADE:
            // In fade mode play a sound for each touch sensor
            if (touch1held == 1) {
                toneAC(NOTE_C3, volume);
            }
            if (touch2held == 1) {
                toneAC(NOTE_D3, volume);
            }
            if (touch3held == 1) {
                toneAC(NOTE_E3, volume);
            }
            if (touch4held == 1) {
                toneAC(NOTE_F3, volume);
            }
            if (touch1held == 0 && touch2held == 0 && touch3held == 0 && touch4held == 0) {
                noToneAC();
            }
            break;
        case MODE_BLINK:
            // In blink mode let the user light up whatever color they desire by pressing sensors (white, red, green, blue)
            // In fade mode play a sound for each touch sensor
            if (touch1held == 1) {
                analogWrite(redPin, maxBrightness);
                analogWrite(greenPin, maxBrightness);
                analogWrite(bluePin, maxBrightness);
                toneAC(NOTE_C4, volume);
            }
            if (touch2held == 1) {
                analogWrite(redPin, maxBrightness);
                toneAC(NOTE_D4, volume);
            }
            if (touch3held == 1) {
                analogWrite(greenPin, maxBrightness);
                toneAC(NOTE_E4, volume);
            }
            if (touch4held == 1) {
                analogWrite(bluePin, maxBrightness);
                toneAC(NOTE_F4, volume);
            }
            if (touch1held == 0 && touch2held == 0 && touch3held == 0 && touch4held == 0) {
                noToneAC();
            }
            break;
        case MODE_STROBE:
            // In fade mode play a sound for each touch sensor
            if (touch1held == 1) {
                toneAC(NOTE_A5, volume);
            }
            if (touch2held == 1) {
                toneAC(NOTE_B5, volume);
            }
            if (touch3held == 1) {
                toneAC(NOTE_C5, volume);
            }
            if (touch4held == 1) {
                toneAC(NOTE_D5, volume);
            }
            if (touch1held == 0 && touch2held == 0 && touch3held == 0 && touch4held == 0) {
                noToneAC();
            }
        case MODE_MANUAL:
            // In manual mode let the user control the colors individually with gentle fading up or down
            RGB color = { touch2held*maxBrightness, touch3held*maxBrightness, touch4held*maxBrightness };
            if (touch1val > touchThreshold) {
                color = White;
            }
            displayColor(color);
            // In fade mode play a sound for each touch sensor
            if (touch1held == 1) {
                toneAC(NOTE_C2, volume);
            }
            if (touch2held == 1) {
                toneAC(NOTE_D2, volume);
            }
            if (touch3held == 1) {
                toneAC(NOTE_E2, volume);
            }
            if (touch4held == 1) {
                toneAC(NOTE_F2, volume);
            }
            if (touch1held == 0 && touch2held == 0 && touch3held == 0 && touch4held == 0) {
                noToneAC();
            }
            break;
    }

// If you need to troubleshoot the touch sensors this will help immensely...
   Serial.print("\t");                    // tab character for debug windown spacing
   Serial.print(touch1val);                // print sensor output 1
   Serial.print("\t");
   Serial.print(touch2val);                  // print sensor output 2
   Serial.print("\t");
   Serial.print(touch3val);                // print sensor output 3
   Serial.print("\t");
   Serial.println(touch4val);                // print sensor output 3
}

// NOTE: Work-in-progress for a white noise generator.  I've got the "noise"
//       part working great already!  I don't think you'd want to fall asleep to
//       the sound generated by this function though...
void whiteNoise(void) {
    toneAC(random(200, 20000), volume, 1);
}

void simonGame(void) {
    // Play the game of Simon!  Eventually.  Doesn't do anything yet.
    if (mode != MODE_SIMON) { // Only do this if we're in fade mode
        return;
    }
    if (simonStarted != 1) {
        simonStarted = 1;
        play(melody);
        playSimon();
    }
}

void playBackground(void) {
    // Plays a given song in the background (async)
    int songLength = sizeof(playingSong) * sizeof(playingSong[0]);
    long noteDuration = (1000/playingSong[songIndex].duration) + 30L;
    long note = playingSong[songIndex].note;
    if (timer.isEnabled(backgroundTimer) == false) {
        timer.enable(backgroundTimer);
    }
    if (lastNoteStart == 0) {
        lastNoteStart = millis();
        toneAC(note, volume, noteDuration); // Play the first note
        songIndex += 1;
        return;
    }
    if (millis() > lastNoteStart + noteDuration) {
        songIndex += 1;
        lastNoteStart = millis();
        noToneAC();
        toneAC(note, volume, noteDuration); // Play the next note
    }
    if (songIndex > songLength) { // Song is done
        songIndex = 0;
        timer.disable(backgroundTimer);
        lastNoteStart = 0;
    }
}

void play(Note* song) {
    playingSong = song;
    playBackground();
}

void playSong(Note* song) {
    // iterate over the notes of the melody:
    int songLength = sizeof(song) * sizeof(song[0]);
    for (int i = 0; i < songLength; i++) {
        // To calculate the note duration, take one second divided by the note type.
        // e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
        int noteDuration = 1000/song[i].duration;
        toneAC(song[i].note);
        //pause for the note's duration plus 30 ms:
        delay(noteDuration + 30);
        noToneAC();
    }
}

// NOTE: I originally used the CapSense library but it wasn't working very well
//       so I switched to this.  Having said that, I believe it wasn't working
//       well because of the sensor wires' original proximity to my darlington
//       array.  So CapSense probably would have worked fine.
// readCapacitivePin
//  Input: Arduino pin number
//  Output: A number, from 0 to 17 expressing
//  how much capacitance is on the pin
//  When you touch the pin, or whatever you have
//  attached to it, the number will get higher
uint8_t readCapacitivePin(int pinToMeasure) {
    // Variables used to translate from Arduino to AVR pin naming
    volatile uint8_t* port;
    volatile uint8_t* ddr;
    volatile uint8_t* pin;
    // Here we translate the input pin number from
    //  Arduino pin number to the AVR PORT, PIN, DDR,
    //  and which bit of those registers we care about.
    byte bitmask;
    port = portOutputRegister(digitalPinToPort(pinToMeasure));
    ddr = portModeRegister(digitalPinToPort(pinToMeasure));
    bitmask = digitalPinToBitMask(pinToMeasure);
    pin = portInputRegister(digitalPinToPort(pinToMeasure));
    // Discharge the pin first by setting it low and output
    *port &= ~(bitmask);
    *ddr  |= bitmask;
    delay(1);
    uint8_t SREG_old = SREG; //back up the AVR Status Register
    // Prevent the timer IRQ from disturbing our measurement
    noInterrupts();
    // Make the pin an input with the internal pull-up on
    *ddr &= ~(bitmask);
    *port |= bitmask;

    // Now see how long the pin to get pulled up. This manual unrolling of the loop
    // decreases the number of hardware cycles between each read of the pin,
    // thus increasing sensitivity.
    uint8_t cycles = 17;
        if (*pin & bitmask) { cycles =  0;}
    else if (*pin & bitmask) { cycles =  1;}
    else if (*pin & bitmask) { cycles =  2;}
    else if (*pin & bitmask) { cycles =  3;}
    else if (*pin & bitmask) { cycles =  4;}
    else if (*pin & bitmask) { cycles =  5;}
    else if (*pin & bitmask) { cycles =  6;}
    else if (*pin & bitmask) { cycles =  7;}
    else if (*pin & bitmask) { cycles =  8;}
    else if (*pin & bitmask) { cycles =  9;}
    else if (*pin & bitmask) { cycles = 10;}
    else if (*pin & bitmask) { cycles = 11;}
    else if (*pin & bitmask) { cycles = 12;}
    else if (*pin & bitmask) { cycles = 13;}
    else if (*pin & bitmask) { cycles = 14;}
    else if (*pin & bitmask) { cycles = 15;}
    else if (*pin & bitmask) { cycles = 16;}

    // End of timing-critical section; turn interrupts back on if they were on before, or leave them off if they were off before
    SREG = SREG_old;

    // Discharge the pin again by setting it low and output
    //  It's important to leave the pins low if you want to
    //  be able to touch more than 1 sensor at a time - if
    //  the sensor is left pulled high, when you touch
    //  two sensors, your body will transfer the charge between
    //  sensors.
    *port &= ~(bitmask);
    *ddr  |= bitmask;

    return cycles;
}

//  NOTE: This Simon game functions are a work-in-progress; based off of code
//        I found elsewhere.  The final result will likely be nothing like the
//        original so I'm not too worried about the copyright.  I just needed an
//        an example of a working game so I don't have to remember the specifics
//        of how Simon worked :).  Don't use this code (it doesn't work yet
//        anyway).
void playSimon(void) {
    for (int y=0; y<=99; y++) {
        //function for generating the array to be matched by the player
        displayColor(White);
        toneAC(SimonRedNote, volume, 50);
        delay(50);
        toneAC(SimonGreenNote, volume, 50);
        delay(50);
        toneAC(SimonBlueNote, volume, 50);
        delay(50);
        toneAC(SimonYellowNote, volume, 50);
        delay(50);
        allOff();
        delay(1000);

        for (int y=turn; y <= turn; y++) { //Limited by the turn variable
            randomArray[y] = random(1, 5); //Assigning a random number (1-4) to the randomArray[y], y being the turn count
            for (int x=0; x <= turn; x++) {
                for(int y=0; y<4; y++) {
                    if (randomArray[x] == 1) {  // if statements to display the stored values in the array
                        displayColor(Red);
                        toneAC(SimonRedNote, volume, 100);
                        delay(400);
                        allOff();
                        delay(100);
                    }
                    if (randomArray[x] == 2) {
                        displayColor(Red);
                        toneAC(SimonGreenNote, volume, 100);
                        delay(400);
                        allOff();
                        delay(100);
                    }
                    if (randomArray[x] == 3) {
                        displayColor(Red);
                        toneAC(SimonBlueNote, volume, 100);
                        delay(400);
                        allOff();
                        delay(100);
                    }
                    if (randomArray[x] == 4) {
                        displayColor(Red);
                        toneAC(SimonYellowNote, volume, 100);
                        delay(400);
                        allOff();
                        delay(100);
                    }
                }
            }
        }
        simonInput();
    }
}
void simonInput(void) { //Function for allowing user input and checking input against the generated array
    for (int x=0; x <= turn;) { //Statement controlled by turn count
        for(int y=0; y<4; y++) {
            if (touch1held == 1) { //Checking for button push
                displayColor(Red);
                toneAC(SimonRedNote, volume, 100);
                delay(200);
                allOff();
                inputArray[x] = 1;
                delay(250);
                if (inputArray[x] != randomArray[x]) { //Checks value input by user and checks it against
                    fail();                              //the value in the same spot on the generated array
                }                                      //The fail function is called if it does not match
                x++;
            }
            if (touch2held == 1) {
                displayColor(Green);
                toneAC(SimonGreenNote, volume, 100);
                delay(200);
                allOff();
                inputArray[x] = 2;
                delay(250);
                if (inputArray[x] != randomArray[x]) {
                    fail();
                }
                x++;
            }
            if (touch3held == 1) {
                displayColor(Blue);
                toneAC(SimonBlueNote, volume, 100);
                delay(200);
                allOff();
                inputArray[x] = 3;
                delay(250);
                if (inputArray[x] != randomArray[x]) {
                    fail();
                }
                x++;
            }
            if (touch4held == 1) {
                displayColor(Yellow);
                toneAC(SimonYellowNote, volume, 100);
                delay(200);
                allOff();
                inputArray[x] = 4;
                delay(250);
                if (inputArray[x] != randomArray[x]) {
                    fail();
                }
                x++;
            }
        }
    }
    delay(500);
    turn++; //Increments the turn count, also the last action before starting the output function over again
}

void fail(void) { //Function used if the player fails to match the sequence
    for (int y=0; y<=2; y++) { //Flashes lights for failure
        displayColor(White);
        toneAC(SimonRedNote, volume, 200);
        delay(200);
        allOff();
        toneAC(NOTE_C3, volume, 200);
        delay(200);
    }
    delay(500);
    turn = -1; //Resets turn value so the game starts over without need for a reset button
}

