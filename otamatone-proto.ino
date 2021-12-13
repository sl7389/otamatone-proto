#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

Adafruit_8x8matrix matrix = Adafruit_8x8matrix();

// GUItool: begin automatically generated code
AudioSynthWaveform       waveform2;      //xy=183,417
AudioSynthWaveform       waveform1;      //xy=204,362
AudioMixer4              mixer1;         //xy=403,407
AudioOutputAnalog        dac1;           //xy=594,433
AudioConnection          patchCord1(waveform2, 0, mixer1, 1);
AudioConnection          patchCord2(waveform1, 0, mixer1, 0);
AudioConnection          patchCord3(mixer1, dac1);
// GUItool: end automatically generated code


//two pots each control note value and velocity
//which will be replaced by a soft pot
//led display will be added

//1 frequency and volume ctrls
//2 waveform change
//3 octave up/down button, quantise
//4 sequencer mode?

//screen:ui
//1 tuner
//2 emoticon show waveform
//3 volume
//octave, quantise


unsigned long mode3StartTime = 0;
unsigned long mode4StartTime = 0;
unsigned long mode5StartTime = 0;
unsigned long mode6StartTime = 0;

//pots
int notePotVal = 0;//for midi
int vPotVal = 0;
int  mappedNotePotVal = 0;
int mappedVPotVal = 0;
//for audio
int freq = 0;
float vol = 0;

float channelVol = 0;
int mappedChannelVol = 0;

//switches and limit
int low = 131;
int high = 262;
int qSwitch = 39;
//for octave buttons
int octUpButton = 37;
int octDownButton = 35;
bool upCurrentState = LOW;
bool downCurrentState = LOW;
bool upLastState = LOW;
bool downLastState = LOW;

bool qCurrentState = LOW;
bool qLastState = LOW;
//matrix state var
int matrixState = 0;


void setup() {
  AudioMemory(12); //always include this when using the Teensy Audio Library even if you don't know what it is yet!

  waveform1.begin(WAVEFORM_SINE);
  waveform2.begin(WAVEFORM_SAWTOOTH);
  waveform1.amplitude(0.1);
  waveform2.amplitude(0.1); //amplitude (volume) can be 0 to 1 and this is a good volume for headphones
  waveform1.frequency(262);
  waveform2.frequency(262);

  Serial.begin(9600);
  matrix.begin(0x70);

  pinMode(octUpButton, INPUT);
  pinMode(octDownButton, INPUT);
  pinMode(qSwitch, INPUT);
}

static const uint8_t PROGMEM
triEmoticon_bmp[] =
{ B00000000,
  B01000010,
  B10100101,
  B00000000,
  B00000000,
  B00111100,
  B00000000,
  B00000000
},
sqEmoticon_bmp[] =
{ B00000000,
  B11100111,
  B10100101,
  B00000000,
  B00000000,
  B00111100,
  B00000000,
  B00000000
},
upArrow_bmp[] =
{
  B00000000,
  B00001000,
  B00011100,
  B00111110,
  B01111111,
  B00011100,
  B00011100,
  B00011100
},
downArrow_bmp[] = {
  B00000000,
  B00011100,
  B00011100,
  B00011100,
  B01111111,
  B00111110,
  B00011100,
  B00001000
};

void loop() {

  notePotVal = analogRead(A12);
  octCheck();
  quantiseCheck();
  setMatrix();

  if (notePotVal > 100) {
    Serial.println(notePotVal);
    vPotVal = analogRead(A7);

    // volume control
    mappedVPotVal = map(vPotVal, 0, 1023, 0, 100);
    vol = mappedVPotVal / 100.0;
    channelVol = analogRead(A9);
    mappedChannelVol = map(channelVol, 0, 1023, 0, 100);
    channelVol = mappedChannelVol / 100.0;
    waveform1.amplitude(vol);

    playNotes();



  } else {
    waveform1.amplitude(0);
    waveform2.amplitude(0);
  }
}

void setMatrix() {
  if (matrixState == 1) { //sine wave; set smiley face
    matrix.clear();
    matrix.drawBitmap(0, 0, sqEmoticon_bmp, 8, 8, LED_ON);
    matrix.writeDisplay();
    //    delay(200);
  } else if (matrixState == 2) {//triangle wave; set angular eyes
    matrix.clear();
    matrix.drawBitmap(0, 0, triEmoticon_bmp, 8, 8, LED_ON);
    matrix.writeDisplay();
    //    delay(200);
  } else if (matrixState == 3) { //quantised
    if (millis() < mode3StartTime + 500) { // check if less than 1 second has passed
      matrix.setTextSize(1);
      matrix.setTextWrap(true);
      matrix.setTextColor(LED_ON);

      matrix.clear();
      matrix.setCursor(0, 0);
      matrix.print("Q");
      matrix.writeDisplay();
      delay(100);
    } else { // if longer, this switches it back to mode 1 or 2
      playNotes();
    }
  } else if (matrixState == 4) { //not quantised
    if (millis() < mode4StartTime + 500) { //check if 1s passed
      matrix.setTextSize(0.5);
      matrix.setTextWrap(true);
      matrix.setTextColor(LED_ON);

      matrix.clear();
      matrix.setCursor(0, 0);
      matrix.print("NQ");
      matrix.writeDisplay();
      delay(100);
    } else { //if longer switch back to mode 1or 2
      playNotes();
    }
  } else if (matrixState == 5) {
    if (millis() < mode5StartTime + 500) {
      matrix.clear();
      matrix.drawBitmap(0, 0, upArrow_bmp, 8, 8, LED_ON);
      matrix.writeDisplay();
    } else {
      playNotes();
    }
  } else if (matrixState == 6) {
    if (millis() < mode6StartTime + 500) {
      matrix.clear();
      matrix.drawBitmap(0, 0, downArrow_bmp, 8, 8, LED_ON);
      matrix.writeDisplay();
    } else {
      playNotes();
    }
  }


}

void quantiseCheck() {
  if (digitalRead(qSwitch) == HIGH) {
    freq = low * pow(2, map(analogRead(A12), 100, 1023, 0, 12) / 12.0);
  } else {
    freq = map(analogRead(A12), 100, 1023, low, high);
  }

  qLastState = qCurrentState;
  qCurrentState = digitalRead(39);
  if (qLastState == LOW and qCurrentState == HIGH) {
    Serial.println("matrixState is 3");
    matrixState = 3; //quantised
    mode3StartTime = millis();

  } else if (qLastState == HIGH and qCurrentState == LOW) {
    Serial.println("matrixState is 4");
    matrixState = 4; //not quantised
    mode4StartTime = millis();

  }
}

void octCheck() {
  upLastState = upCurrentState;
  upCurrentState = digitalRead(37);
  downLastState = downCurrentState;
  downCurrentState = digitalRead(35);
  if (upLastState == LOW and upCurrentState == HIGH and high <= 2094) {
    //highest note is C7?
    low = low * 2;
    high = high * 2;
    mode5StartTime = millis();
    matrixState = 5;
  } else {
    low = low;
    high = high;
  }
  if (downLastState == LOW and downCurrentState == HIGH and low > 130) {
    //lowest note is C3?
    low = low / 2;
    high = high / 2;
    mode6StartTime = millis();
    matrixState = 6;
  } else {
    low = low;
    high = high;

  }

}

void playNotes() {

  waveform1.frequency(freq);//sine
  waveform2.frequency(freq);
  waveform1.amplitude(vol);
  waveform2.amplitude(vol);
  mixer1.gain(0, channelVol);//sine
  mixer1.gain(1, 1.0 - channelVol);//sawt

  if (channelVol <= 0.5) {
    matrixState = 1;
  } else if (channelVol > 0.5) {
    matrixState = 2;
  }

}
