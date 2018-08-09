#include <FastLED.h>
#include <EEPROM.h>


// declare global variables here
const long minute = 60000; // 1 minute in ms
unsigned long currentTime;
unsigned long previousTime = 0;
byte currentMinute = 0;
byte currentHour = 12;
byte am = 1; // 0 = false 1 = true;

char* firstDigit;
char* secondDigit;
char* thirdDigit;
char* fourthDigit;

//protoypes
void incrementMinute();
void incrementHour();
int setDigit(int clockDigit, int setTo);
char* getDigitArray(int clockDigit);
void printDigits();
void printTime();
void setTime();
void setAM();
void setPM();
void checkForCommand();
void changeTime();
void getLit();

void setup() {
  //use for debugging
  Serial.begin(9600);
//  check if serial port is connected:
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("startup begin");

  // set pins:
  pinMode(LED_BUILTIN, OUTPUT);
  for (int i = 2; i <= 13; i++)
    pinMode(i, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // initalize memory for didits
  firstDigit = (char*)malloc(sizeof(char) * 7);
  secondDigit = (char*)malloc(sizeof(char) * 7);
  thirdDigit = (char*)malloc(sizeof(char) * 7);
  fourthDigit = (char*)malloc(sizeof(char) * 7);
  setDigit(1, 1);
  setDigit(2, 2);
  setDigit(3, 0);
  setDigit(4, 0);
  setPM();

  // get time from EEPROM
  currentHour = EEPROM.read(0);
  currentMinute = EEPROM.read(1);
  am = EEPROM.read(2);
  // TODO: load in LEDs from EEPROM
  digitalWrite(LED_BUILTIN, HIGH);
  printTime();
  Serial.println("startup end");
}

void loop() {
  // get current time
  currentTime = millis();
  // currentently accounts for millis() overflow by calculating duration
  if (currentTime - previousTime >= minute) {
    // TODO: CHANGE but rn just now blink  builtin light
    if (digitalRead(LED_BUILTIN) == HIGH) {
      digitalWrite(LED_BUILTIN, LOW);
    }
    else {
      digitalWrite(LED_BUILTIN, HIGH);
    }
    // TODO: END OF NEEDED CHANGE
    incrementMinute();
    printTime();

    // last thing to do
    previousTime = currentTime;
  }
  checkForCommand(); // used to set the time
  getLit();
}

void changeTime() {
  setTime();
  previousTime = currentTime;
  printTime();
}

void incrementMinute() {
  currentMinute++; currentMinute %= 60; // return to 0 if we increment
  if (currentMinute == 0) {
    incrementHour();
  }
  setTime();
}

void incrementHour() {
  currentHour++; currentHour %= 12;
  if (currentHour == 0) {
    currentHour = 12;
    am = !am;
  }
}

void setTime() {
  // set the clock digits
  setDigit(1, currentHour / 10);
  setDigit(2, currentHour - (currentHour / 10));
  setDigit(3, currentMinute / 10);
  setDigit(4, currentMinute - (currentMinute / 10));
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
 * return 0 on success and 1 on failure
 */
int setDigit(int clockDigit, int setTo) {
    // verify we have good parameters
    if (setTo < 0 || setTo > 9)
        return 1;
    if (clockDigit == 1 && (setTo < 0 || setTo > 1))
        return 1;

    char* array = getDigitArray(clockDigit);

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
            array[5] = '1';
            array[6] = '0';
            break;
        case 2:
            array[0] = '1';
            array[1] = '0';
            array[2] = '1';
            array[3] = '1';
            array[4] = '1';
            array[5] = '0';
            array[6] = '1';
            break;
        case 3:
            array[0] = '1';
            array[1] = '0';
            array[2] = '1';
            array[3] = '1';
            array[4] = '0';
            array[5] = '1';
            array[6] = '1';
            break;
        case 4:
            array[0] = '0';
            array[1] = '1';
            array[2] = '1';
            array[3] = '1';
            array[4] = '0';
            array[5] = '1';
            array[6] = '0';
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
            array[0] = '1';
            array[1] = '0';
            array[2] = '1';
            array[3] = '0';
            array[4] = '0';
            array[5] = '1';
            array[6] = '0';
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
            array[6] = '0';
            break;
        default:
            return 1;
            break;
    }
    return 0;
}

char* getDigitArray(int clockDigit) {
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
    return firstDigit;
}

void checkForCommand() {
  if (Serial.available() > 0)
  {
    String input = Serial.readString();
    input.trim(); // remove leading and trailing whitespace
    int i = 0;
    while (input.charAt(i) != ':' && input.charAt(i) != '\0' && input.charAt(i) != '\n') {
      i++;
    }
    if (input.substring(0, i).equals("time")) {
      input.remove(0, i+1);
      input.replace(" ", "");
      input.toLowerCase();
      // if 1:23pm
      if (input.charAt(1) == ':') {
        currentHour = (byte)input.substring(0, 1).toInt();
        currentMinute = (byte)input.substring(2,4).toInt();
        Serial.println("1 = " + currentHour);
        Serial.println("2 = " + currentMinute);

        if (input.substring(4,6).equals("am"))
          am = 1;
        else
          am = 0;
      }
      // if 12:34pm
      else {
        currentHour = (byte)input.substring(0, 2).toInt();
        currentMinute = (byte)input.substring(3,5).toInt();
        if (input.substring(5,7).equals("am"))
          am = 1;
        else
          am = 0;
      }
      //potentially override time here:
      EEPROM.update(0, currentHour); // hour
      EEPROM.update(1, currentMinute); // minute
      EEPROM.update(2, am); // 1 for am 0 for pm
      Serial.println(input);
      changeTime();
    }
    // command format: LED: LED#, redValue, greenValue, blueValue
    else if (input.substring(0, i).equals("LED")) {
      input.replace(" ", "");
      input.remove(0, i+1); // remove command
      // get args
      i = 0;
      while (input.charAt(i) != ',') {
        i++;
      }
      int ledNum = input.substring(0, i).toInt();
      input.remove(0, i+1);
      i = 0;
      while (input.charAt(i) != ',') {
        i++;
      }
      int red = input.substring(0, i).toInt();
      input.remove(0, i+1);
      i = 0;
      while (input.charAt(i) != ',') {
        i++;
      }
      int green = input.substring(0, i).toInt();
      input.remove(0, i+1);
      int blue = input.toInt();
      //TODO: PUT IN EEPROM
    }
  }
}

void getLit() {
  for (int i = 1; i <= 4; i++) {
    char* array = getDigitArray(i);
    //TODO:
  }
}

void printTime() {
  Serial.print("current time = ");
  Serial.print(currentHour);
  Serial.print(":");
  Serial.print(currentMinute);
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
