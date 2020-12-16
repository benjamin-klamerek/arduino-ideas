#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <avr/sleep.h>

//All variables used by SDPlayer module
SoftwareSerial serialForMP3Player(10, 11); // RX, TX
const bool gc_useMP3Ack = true;
const bool gc_resetMP3OnBoot = true;
const unsigned short gc_MP3Volume = 5; //volume, between 0 and 30
DFRobotDFPlayerMini myDFPlayer;
const unsigned short GC_TONE_WAITING    = 1;
const unsigned short GC_TONE_OCCUPIED   = 2;
const unsigned short GC_TONE_SEARCHING  = 3;
const unsigned short GC_TONE_RINGING    = 4;
const unsigned short GC_TONE_ERROR      = 5;
const unsigned short MP3_NOT_PLAYING = DFPlayerPlayFinished;
const unsigned short MP3_PLAYING = 513;
const int INTPUT_PIN_PLAYER_BUSY = 5;

//Pins used for rotary phone
const int INTPUT_PIN_HANG_UP = 2;
const int INTPUT_PIN_LEG_1 = 3;
const int INTPUT_PIN_LEG_3 = 4;

//Depending of rotary sensor, you may adjust the default 66ms value
const unsigned int pulseInterval = 66 + 20;

//States
const unsigned short STATUS_STARTING     = 0;
const unsigned short STATUS_COMPOSING    = 1;
const unsigned short STATUS_CALLING      = 2;

//Mutable state variables
int currentStatus = STATUS_STARTING;
volatile unsigned long dialTime = 0;
volatile unsigned long lastDialTime = 0;
volatile short currentDigitComposed = -1;
String numberDialed = "";
volatile bool composingDigit = false;

/**
   Count number of signal sent by rotary sensor
*/
void rotaryPulseCallback() {
  composingDigit = true;
  dialTime = millis();
  if (dialTime - lastDialTime >= pulseInterval) {
    currentDigitComposed++;
    lastDialTime = dialTime;
  }
}


void wakeUp() {
  digitalWrite(LED_BUILTIN, HIGH);
}

/**
   Reset current state (stip playing sound, current number, ...)
*/
void stopMusicAndClearNumber() {
  myDFPlayer.pause();
  clearNumber();
  clearComposedDigit();
  currentStatus = STATUS_STARTING;
}

void clearNumber() {
  numberDialed = "";
}

void clearComposedDigit() {
  currentDigitComposed = -1;
}

/**
   True if rotary sensor is actually used, false otherwise
*/
bool isRotaring() {
  int rotaringPinStatus;
  do {
    rotaringPinStatus = digitalRead(INTPUT_PIN_LEG_3);
  } while (rotaringPinStatus != digitalRead(INTPUT_PIN_LEG_3));
  return rotaringPinStatus == LOW;
}

bool isPlaying() {
  int playingStatus = digitalRead(INTPUT_PIN_PLAYER_BUSY);
  return playingStatus == LOW;
}

/*
   Print the detail message from DFPlayer to handle different errors and states (useful in debug mode)
*/
void printDetail(uint8_t type, int value) {
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      Serial.println(F("NONE"));
      break;
  }
}

void setup() {
  serialForMP3Player.begin(9600);
  Serial.begin(115200);
  pinMode(INTPUT_PIN_HANG_UP, INPUT_PULLUP);
  pinMode(INTPUT_PIN_LEG_1, INPUT_PULLUP);
  pinMode(INTPUT_PIN_LEG_3, INPUT_PULLUP);
  pinMode(INTPUT_PIN_PLAYER_BUSY, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(INTPUT_PIN_LEG_1), rotaryPulseCallback, RISING);

  delay(2000); // MP3 player seem to need time to boot.
  // Use softwareSerial to communicate with mp3,
  if ( ! myDFPlayer.begin(serialForMP3Player, true, true) ) {
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    Serial.println(F("3.If it still fail : try to change MP3 flags in configuration"));
    while (true);
  }
  Serial.println(F("MP3 player ready"));
  delay(1000); // and more time to listen to its serial port

  //----Setup MP3 player ----
  //Set serial communication time out 800ms (bellow seems to create errors when getting SD files number)
  myDFPlayer.setTimeOut(800);
  myDFPlayer.volume(gc_MP3Volume);
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  Serial.println("Socotel Ready !");

}

void loop() {

  if (myDFPlayer.available()) {
    printDetail(myDFPlayer.readType(), myDFPlayer.read());
  }

  int sensorVal = digitalRead(INTPUT_PIN_HANG_UP);
  if (sensorVal == HIGH) {
    digitalWrite(LED_BUILTIN, LOW);
    stopMusicAndClearNumber();
    detachInterrupt(digitalPinToInterrupt(INTPUT_PIN_LEG_1));
    attachInterrupt(digitalPinToInterrupt(INTPUT_PIN_HANG_UP), wakeUp, LOW);
    delay(100);

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_mode();
    sleep_disable();
    detachInterrupt(digitalPinToInterrupt(INTPUT_PIN_HANG_UP));
    attachInterrupt(digitalPinToInterrupt(INTPUT_PIN_LEG_1), rotaryPulseCallback, RISING);
  }

  if (! isRotaring() && composingDigit) {
    short digit = (currentDigitComposed + 1) % 10;
    numberDialed += digit;
    Serial.println(numberDialed);
    clearComposedDigit();
    composingDigit = false;
  }

  int numberLength = numberDialed.length();
  if (numberLength < 7 && (currentStatus == STATUS_STARTING
                           || (currentStatus == STATUS_COMPOSING && !isPlaying())
                          )) {
    myDFPlayer.playMp3Folder(GC_TONE_WAITING);
    currentStatus = STATUS_COMPOSING;
  } else if (numberLength >= 7 && currentStatus == STATUS_COMPOSING) {
    currentStatus = STATUS_CALLING;
    myDFPlayer.playMp3Folder(GC_TONE_SEARCHING);
    delay(3000);
    myDFPlayer.playMp3Folder(GC_TONE_RINGING);
    delay(7000);
    //If you want to configure number -> song : it's here !!!
    if (numberDialed.equals("5552333")) {
      myDFPlayer.playMp3Folder(6);
      delay(3000);
    } else {
      myDFPlayer.playMp3Folder(GC_TONE_ERROR);
      delay(10000);
    }
  }


  delay(300);

}
