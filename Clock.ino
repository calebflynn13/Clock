#include <FastLED.h>
#include <EEPROM.h>
#include <Wire.h>
#include "ds3231.h"

#define LED_PIN     12
#define NUM_LEDS    98
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

#define TIMER_ALERT_TIMEOUT 10


// declare global variables here
#define HOUR   3600000
#define MINUTE 60000
#define SECOND 1000

CRGB leds[NUM_LEDS];
byte colors[NUM_LEDS * 3];
byte brightness = 192;

unsigned long currentTime;
unsigned long previousTime = 0;
byte currentHour = 12;
byte currentMinute = 0;
byte currentSecond = 0;
byte am = 1; // 0 = false 1 = true;

byte startUpMode = 0; // 1 = true, 0 = false;

byte animation = 0; // which animation we are playing
unsigned long animationStart; // what time the animation starts

byte timerMode = 0; // 1 = true, 0 = false
unsigned long timerPrev;
long timerLength = 0; // keep it signed so we can detect when it crosses 0 threshold
byte timerAm = 0;

char* firstDigit;
char* secondDigit;
char* thirdDigit;
char* fourthDigit;

//protoypes
void incrementMinute();
void incrementHour();
int setDigit(int clockDigit, int setTo, byte clockType);
char* getDigitArray(int clockDigit, byte clockType);
void printDigits();
void printTime();
void setTime();
void setTimer(byte hour, byte minute, byte second);
void setAM();
void setPM();
void checkForCommand();
void changeTime();
void getLit();

void setup() {
  // use for debugging
  Serial.begin(9600);
  // TODO: check if serial port is connected:
  Serial.println("startup begin");

  // set pins:
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(12, OUTPUT);

  // setup the rtc
  Wire.begin();
  DS3231_init(DS3231_CONTROL_INTCN);

  // initalize memory for didits
  firstDigit = (char*)malloc(sizeof(char) * 7);
  secondDigit = (char*)malloc(sizeof(char) * 7);
  thirdDigit = (char*)malloc(sizeof(char) * 7);
  fourthDigit = (char*)malloc(sizeof(char) * 7);
  setDigit(1, 1, 0);
  setDigit(2, 2, 0);
  setDigit(3, 0, 0);
  setDigit(4, 0, 0);
  setPM();

  // get time from EEPROM
  currentHour = 12;
  currentMinute = 0;
  currentSecond = 0;
  am = 0;
  brightness = (byte) EEPROM.read(0);
  setTime(); // set to the real time
  timerMode = 0; // only sent over command

  delay(3000); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(brightness);
  printTime();
  timerMode = 0; // only sent over command

  // read in colors from memory
  for (int i = 0; i < NUM_LEDS; i++) {
    colors[i * 3]     = (byte) EEPROM.read(i * 3 + 3); // red
    colors[i * 3 + 1] = (byte) EEPROM.read(i * 3 + 4); // green
    colors[i * 3 + 2] = (byte) EEPROM.read(i * 3 + 5); // blue
  }
  // read in animation variable from memory
  animation = EEPROM.read(NUM_LEDS * 3 + 3);

  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("startup end");
}

void loop() {
  // get current time
  currentTime = millis();
  struct ts t;
  
  if (currentTime - previousTime >= SECOND) {
    DS3231_get(&t);
    
    // set the hour
    if (t.hour > 12) {
      am = 1;
      currentHour = t.hour % 12;
    }
    else {
      am = 0;
      currentHour = t.hour;
    }
    
    // set the minute
    currentMinute = t.min;
    
    // set the seconds
    currentSecond = t.sec;
    
    // only change the time if we are in clock mode
    if (timerMode == 0) {
      changeTime();
    }    
    // last thing to do
    previousTime = currentTime;
  }
  if (timerMode) {
    timerLength -= (currentTime - timerPrev); // get total milliseconds left
    timerPrev = currentTime;
    byte hour = (byte)(timerLength /  HOUR);  // get hours left
    byte minute = (byte)((timerLength - (hour * HOUR)) / MINUTE); // get minutes left
    byte second = (byte)((timerLength - (hour * HOUR) - (minute * MINUTE)) / SECOND); // get seconds left
    Serial.print("timerLength = ");
    Serial.print(timerLength);
    Serial.print("  time left = ");
    Serial.print(hour);
    Serial.print(":");
    Serial.print(minute);
    Serial.print(":");
    Serial.println(second);
    if (timerLength <= 0) {
      byte halfSecond = ((timerLength - (hour * HOUR) - (minute * MINUTE)) / 500);      
      if (halfSecond % 2 == 0) {
        setDigit(1, -1, 1);
        setDigit(2, -1, 1);
        setDigit(3, -1, 1);
        setDigit(4, -1, 1);
      }
      else {
        setDigit(1, 0, 1);
        setDigit(2, 0, 1);
        setDigit(3, 0, 1);
        setDigit(4, 0, 1);
      }
      if (timerLength <= -1 * TIMER_ALERT_TIMEOUT * SECOND) {
        timerMode = 0; // reset to clock mode
        setTime(); // return digit arrays back to clock mode
      }
    }
    else {
      setTimer(hour, minute, second);
    }
  }
  checkForCommand(); // used to set the time
  getLit();
  delay(30); // slow down, don't hurt yourself
}

void changeTime() {
  setTime();
  printTime();
}

void setTime() {
  // set the clock digits
  setDigit(1, currentHour / 10, 0);
  setDigit(2, currentHour - 10 * (currentHour / 10), 0);
  setDigit(3, currentMinute / 10, 0);
  setDigit(4, currentMinute - 10 * (currentMinute / 10), 0);
}

void setTimer(byte hour, byte minute, byte second) {
  // case 1: minute > 20
  if (minute >= 20 || hour > 0) {
    setDigit(1, hour / 10, 1);
    setDigit(2, hour - 10 * (hour / 10), 1);
    setDigit(3, minute / 10, 1);
    setDigit(4, minute - 10 * (minute / 10), 1);
    if (second % 2 == 0) {
      timerAm = 1;
    }
    else {
      timerAm = 0;
    }
  }
  else { // 19 or less minutes left, put minutes in left side, seconds on right
    setDigit(1, minute / 10, 1);
    setDigit(2, minute - 10 * (minute / 10), 1);
    setDigit(3, second / 10, 1);
    setDigit(4, second - 10 * (second / 10), 1);
  }
}

void setAM() {
  am = 1;
}
void setPM() {
  am = 0;
}

/**
 * inputs:
 *     clockDigit: which digit are we editing? 1, 2, 3, or 4 from left to right
 *     setTo:      the value we want that clockDigit to be set to 0-12 for most
 *     clockType:  0 = normal clock, 1 = timer clock
 * return 0 on success and 1 on failure
 */
int setDigit(int clockDigit, int setTo, byte clockType) {
  char* array = getDigitArray(clockDigit, clockType);
  if (clockDigit == 1) {
    // it can only be a 0 or a 1
    switch (setTo){
      case 0:
        array[0] = '0';
        array[1] = '0';
        array[2] = '0';
        array[3] = '0';
        array[4] = '0';
        array[5] = '0';
        array[6] = '0';
        break;
      case 1:
        array[0] = '0';
        array[1] = '0';
        array[2] = '1';
        array[3] = '0';
        array[4] = '0';
        array[5] = '0';
        array[6] = '1';
        break;
    }
    return 0;
  }
  switch (setTo) {
    case 0:
      array[0] = '1';
      array[1] = '1';
      array[2] = '1';
      array[3] = '0';
      array[4] = '1';
      array[5] = '1';
      array[6] = '1';
      break;
    case 1:
      array[0] = '0';
      array[1] = '0';
      array[2] = '1';
      array[3] = '0';
      array[4] = '0';
      array[5] = '0';
      array[6] = '1';
      break;
    case 2:
      array[0] = '0';
      array[1] = '1';
      array[2] = '1';
      array[3] = '1';
      array[4] = '1';
      array[5] = '1';
      array[6] = '0';
      break;
    case 3:
      array[0] = '0';
      array[1] = '1';
      array[2] = '1';
      array[3] = '1';
      array[4] = '0';
      array[5] = '1';
      array[6] = '1';
      break;
    case 4:
      array[0] = '1';
      array[1] = '0';
      array[2] = '1';
      array[3] = '1';
      array[4] = '0';
      array[5] = '0';
      array[6] = '1';
      break;
    case 5:
      array[0] = '1';
      array[1] = '1';
      array[2] = '0';
      array[3] = '1';
      array[4] = '0';
      array[5] = '1';
      array[6] = '1';
      break;
    case 6:
      array[0] = '1';
      array[1] = '1';
      array[2] = '0';
      array[3] = '1';
      array[4] = '1';
      array[5] = '1';
      array[6] = '1';
      break;
    case 7:
      array[0] = '0';
      array[1] = '1';
      array[2] = '1';
      array[3] = '0';
      array[4] = '0';
      array[5] = '0';
      array[6] = '1';
      break;
    case 8:
      array[0] = '1';
      array[1] = '1';
      array[2] = '1';
      array[3] = '1';
      array[4] = '1';
      array[5] = '1';
      array[6] = '1';
      break;
    case 9:
      array[0] = '1';
      array[1] = '1';
      array[2] = '1';
      array[3] = '1';
      array[4] = '0';
      array[5] = '1';
      array[6] = '1';
      break;
    case -1:
      array[0] = '0';
      array[1] = '0';
      array[2] = '0';
      array[3] = '0';
      array[4] = '0';
      array[5] = '0';
      array[6] = '0';
    default:
      return 1;
      break;
  }
  return 0;
}

char* getDigitArray(int clockDigit, byte clockType) {
    switch(clockDigit) {
      case 1:
        return firstDigit;
        break;
      case 2:
        return secondDigit;
        break;
      case 3:
        return thirdDigit;
        break;
      case 4:
        return fourthDigit;
        break;
    }
    return firstDigit; // default case
}



void getLit() {
  // set to proper colors:
  if (animation > 0) {
    if (animation == 1) {
      // do solid rainbow transition animation
      float progress = (float) (((currentTime - animationStart) % 24000) / 24000.0);
      byte fractionProgress = (byte) (progress * 6); // 0 - 5
      for (int i = 0; i < NUM_LEDS; i++) {
        if (fractionProgress == 0) {
          leds[i].red = 255;
          leds[i].green = (byte) ((progress * 6.0) * 255.0); // increase green
          leds[i].blue = 0;
        }
        else if (fractionProgress == 1) {
          leds[i].red = (byte) (255 - (((progress * 6.0) - 1.0) * 255)); // decrease red
          leds[i].green = 255;
          leds[i].blue = 0;
        }
        else if (fractionProgress == 2) {
          leds[i].red = 0;
          leds[i].green = 255;
          leds[i].blue = (byte) (((progress * 6.0) - 2.0) * 255);// increase blue
        }
        else if (fractionProgress == 3) {
          leds[i].red = 0;
          leds[i].green = (byte) (255 - (((progress * 6.0) - 3.0) * 255)); // decrease green
          leds[i].blue = 255;
        }
        else if (fractionProgress == 4) {
          leds[i].red = (byte) (((progress * 6.0) - 4.0) * 255); // increase red
          leds[i].green = 0;
          leds[i].blue = 255;
        }
        else if (fractionProgress >= 5) {
          leds[i].red = 255;
          leds[i].green = 0;
          leds[i].blue = (byte) (255 - (((progress * 6.0) - 5.0) * 255)); // decrease blue
        }
      }
    }
  }
  else {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].red = getRed(i);
      leds[i].green = getGreen(i);
      leds[i].blue = getBlue(i);
    }
  }

  // turn ones we don't want off for timer or clock depending on mode
  // first digit : LEDs 0 - 7
  if ((getDigitArray(1, timerMode))[6] == '0') {
    leds[0] = CRGB::Black;
    leds[1] = CRGB::Black;
    leds[2] = CRGB::Black;
    leds[3] = CRGB::Black;
  }
  if ((getDigitArray(1, timerMode))[2] == '0') {
    leds[4] = CRGB::Black;
    leds[5] = CRGB::Black;
    leds[6] = CRGB::Black;
    leds[7] = CRGB::Black;
  }

  // second digit : LEDs 8 - 35
  for (int i = 0; i < 7; i++) {
    if ((getDigitArray(2, timerMode))[i] == '0') {
      leds[i * 4 + 8 ] = CRGB::Black;
      leds[i * 4 + 9 ] = CRGB::Black;
      leds[i * 4 + 10] = CRGB::Black;
      leds[i * 4 + 11] = CRGB::Black;
    }
  }

  // leave two colons as is.

  // third digit : LEDs 38 - 65
  for (int i = 0; i < 7; i++) {
    if ((getDigitArray(3, timerMode))[i] == '0') {
      leds[i * 4 + 38] = CRGB::Black;
      leds[i * 4 + 39] = CRGB::Black;
      leds[i * 4 + 40] = CRGB::Black;
      leds[i * 4 + 41] = CRGB::Black;
    }
  }

  // fourth digit : LEDs 66 - 93
  for (int i = 0; i < 7; i++) {
    if ((getDigitArray(4, timerMode))[i] == '0') {
      leds[i * 4 + 66] = CRGB::Black;
      leds[i * 4 + 67] = CRGB::Black;
      leds[i * 4 + 68] = CRGB::Black;
      leds[i * 4 + 69] = CRGB::Black;
    }
  }

  // am/pm
  if (timerMode) {
    if (timerAm == 0) {
      leds[94] = CRGB::Black;
      leds[95] = CRGB::Black;
    }
    else if (timerAm == 1) {
      leds[96] = CRGB::Black;
      leds[97] = CRGB::Black;
    }
  }

  else {
    if (am == 0) {
      leds[94] = CRGB::Black;
      leds[95] = CRGB::Black;
    }
    else if (am == 1) {
      leds[96] = CRGB::Black;
      leds[97] = CRGB::Black;
    }
  }

  if (startUpMode == 0) {
    FastLED.show();
  } else {
    // do nothing
  }
}

byte getRed(int i) {
  return colors[i * 3]; // red part of LED in memory
}

byte getGreen(int i) {
  return colors[i * 3 + 1]; // green part of LED
}

byte getBlue(int i) {
  return colors[i * 3 + 2]; // blue part of LED in memory
}


void checkForCommand() {
  if (Serial.available() > 0)
  {
    Serial.flush();
    String input = Serial.readString();
    //String input = Serial.readStringUntil('\n');
    input.trim(); // remove leading and trailing whitespace
    Serial.print("input = ");
    Serial.println(input);
    int i = 0;
    while (input.charAt(i) != ':' && input.charAt(i) != '\0' && input.charAt(i) != '\n') {
      i++;
    }
    if (input.substring(0, i).equals("1")) { // time:
      // format 1:12:34:56:am
      input.remove(0, i+1);
      input.replace(" ", "");
      input.toLowerCase();

      // get hour
      byte j = 0;
      while (input.charAt(j) != ':' && input.charAt(j) != '\0' && input.charAt(j) != '\n') {
        j++;
      }
      currentHour = (byte)input.substring(0, j).toInt();
      input.remove(0, j + 1); // remove hour plus ':'

      // get minute
      j = 0;
      while (input.charAt(j) != ':' && input.charAt(j) != '\0' && input.charAt(j) != '\n') {
        j++;
      }
      currentMinute = (byte)input.substring(0, j).toInt();
      input.remove(0, j + 1); // remove minute plus ':'

      // get second
      j = 0;
      while (input.charAt(j) != ':' && input.charAt(j) != '\0' && input.charAt(j) != '\n') {
        j++;
      }
      currentSecond = (byte)input.substring(0, j).toInt();
      input.remove(0, j + 1); // remove seconds plus ':'

      if (input.substring(0, 2).equals("am")) {
        am = 1;
      }
      else {
        am = 0;
      }

      Serial.print("hour = ");
      Serial.println(currentHour);
      Serial.print("minute = ");
      Serial.println(currentMinute);
      Serial.print("second = ");
      Serial.println(currentSecond);

      //potentially override time here:
//      EEPROM.update(0, currentHour); // hour
//      EEPROM.update(1, currentMinute); // minute
//      EEPROM.update(2, am); // 1 for am 0 for pm
      Serial.println(input);
      changeTime();
      Serial.println("a"); // confirm that command has completed
    }
    // command format: LED: LED#, redValue, greenValue, blueValue
    else if (input.substring(0, i).equals("2")) { // LED:
      Serial.println("got an LED command");
      animation = 0; // not an animation any more
      EEPROM.update(NUM_LEDS * 3 + 3, animation);
      input.replace(" ", "");
      input.remove(0, i+1); // remove command
//      for (int j = 0; j < input.length(); j++) { // previously used to print rgb values
//        Serial.print((byte)input.charAt(j));
//        Serial.print(",");
//      }
      // get args
      //start loop
      for (int j = 0; j < NUM_LEDS; j++){ // go through all LEDs
        byte ledNum = j;
        byte red = (byte)(input.charAt(j * 3));
        byte green = (byte)(input.charAt(j * 3 + 1));
        byte blue = (byte)(input.charAt(j * 3 + 2));

        Serial.print("LED: ");
        Serial.print(ledNum);
        Serial.print(", ");
        Serial.print("red = ");
        Serial.print(red);
        Serial.print(", ");
        Serial.print("green = ");
        Serial.print(green);
        Serial.print(", ");
        Serial.print("blue = ");
        Serial.println(blue);

        EEPROM.update(ledNum * 3 + 3, (byte)red);
        EEPROM.update(ledNum * 3 + 4, (byte)green);
        EEPROM.update(ledNum * 3 + 5, (byte)blue);
        colors[ledNum * 3] = (byte)red;
        colors[ledNum * 3 + 1] = (byte)green;
        colors[ledNum * 3 + 2] = (byte)blue;

      }
      // end loop
      Serial.println("a"); // confirm that command has completed
    }
    else if (input.substring(0, i).equals("3")) { // preset animations
      input.remove(0, i+1);
      input.replace(" ", "");
      animation = (byte)input.substring(0).toInt(); // get animation #
      EEPROM.update(NUM_LEDS * 3 + 3, animation);
      Serial.print("animation # = ");
      Serial.println(animation);
      if (animation == 1) {
        for (int j = 0; j < NUM_LEDS; j++) {
          leds[j] = 0xFF0000;
        }
      }

      animationStart = millis();
      Serial.println("a"); // confirm that command has completed
    }
    else if (input.substring(0, i).equals("4")) { // turn LEDs off
      Serial.println("turning LEDs off");
      startUpMode = 1;
      FastLED.clear();
      Serial.println("a"); // confirm that command has completed
    }
    else if (input.substring(0, i).equals("5")) { // turn LEDs on
      Serial.println("turning LEDs on");
      startUpMode = 0;
      Serial.println("a"); // confirm that command has completed
    }
    else if (input.substring(0, i).equals("6")) { // set timer
      // format: "6: hh:mm:ss"
      input.replace(" ", "");
      input.remove(0, i+1); // remove command
      // get hour
      int j = 0;
      while (input.charAt(j) != ':' && input.charAt(j) != '\0' && input.charAt(j) != '\n') {
        j++;
      }
      int hour = input.substring(0, j).toInt();
      input.remove(0, j + 1); // remove hour and proceeding colon
      j = 0;
      while (input.charAt(j) != ':' && input.charAt(j) != '\0' && input.charAt(j) != '\n') {
        j++;
      }
      int minute = input.substring(0, j).toInt();
      input.remove(0, j + 1); // remove minute and proceeding colon
      j = 0;
      while (input.charAt(j) != ':' && input.charAt(j) != '\0' && input.charAt(j) != '\n') {
        j++;
      }
      int second = input.substring(0, j).toInt();
      setTimer(hour, minute, second);
      Serial.print("timer set for ");
      Serial.print(hour);
      Serial.print(":");
      Serial.print(minute);
      Serial.print(":");
      Serial.println(second);

      // set timerLength
      timerLength = hour * 3600000;  // add hours in milliseconds
      timerLength += minute * 60000; // add minutes in milliseconds
      timerLength += second * 1000;  // add seconds in milliseconds
      Serial.print("timer length = ");
      Serial.println(timerLength);

      timerPrev = millis();
      timerMode = 1;
      Serial.println("a"); // confirm that command has completed
    }
    else if (input.substring(0, i).equals("7")) {
      Serial.println("put in clock mode");
      timerMode = 0;
      setTime(); // return digits back to normal
      Serial.println("a"); // confirm that command has completed
    }
    else if (input.substring(0, i).equals("8")) {
      input.replace(" ", "");
      input.remove(0, i+1); // remove command
      // get brightness
      int j = 0;
      while (input.charAt(j) != ':' && input.charAt(j) != '\0' && input.charAt(j) != '\n') {
        j++;
      }
      brightness = (byte)input.substring(0, j).toInt();
      EEPROM.update(0, brightness);
      FastLED.setBrightness(brightness); // set the led brightness
      Serial.println("a"); // confirm that command has completed
    }
    else if (input.substring(0, i).equals("9")) { // debugging
      // print LED EEPROM
      for (int j = 0; j < NUM_LEDS; j++) {
        Serial.print("LED: ");
        Serial.print(j);
        Serial.print(", ");
        Serial.print("red = ");
        Serial.print(EEPROM.read(j * 3 + 3));
        Serial.print(", ");
        Serial.print("green = ");
        Serial.print(EEPROM.read(j * 3 + 4));
        Serial.print(", ");
        Serial.print("blue = ");
        Serial.println(EEPROM.read(j * 3 + 5));
      }
      Serial.print("timeMode = ");
      Serial.println(timerMode);
      Serial.print("animation = ");
      Serial.println(animation);
    }
  }
}

void printTime() {
  Serial.print("current time = ");
  Serial.print(currentHour);
  Serial.print(":");
  Serial.print(currentMinute);
  Serial.print(":");
  Serial.print(currentSecond);
  if (am)
    Serial.println(" AM");
  else
    Serial.println(" PM");
  // printDigits();
}

void printDigits() {
    // print variables:
    Serial.print("Digit 1: ");
    for (int i = 0; i < 7; i++) {
      Serial.print(firstDigit[i]);
      Serial.print(", ");
    }
    Serial.print("\n");

    Serial.print("Digit 2: ");
    for (int i = 0; i < 7; i++) {
      Serial.print(secondDigit[i]);
      Serial.print(", ");
    }
    Serial.print("\n");

    Serial.print("Digit 3: ");
    for (int i = 0; i < 7; i++) {
      Serial.print(thirdDigit[i]);
      Serial.print(", ");
    }
    Serial.print("\n");

    Serial.print("Digit 4: ");
    for (int i = 0; i < 7; i++) {
      Serial.print(fourthDigit[i]);
      Serial.print(", ");
    }
    Serial.print("\n");
}

int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
