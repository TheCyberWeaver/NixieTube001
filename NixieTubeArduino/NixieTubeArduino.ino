#include <SoftwareSerial.h>
#include <DS3231.h>
//global

#define dotA 12
#define dotB 10
#define dotC 9
#define dotD 11

#define SR_latchPIN 2
#define SR_clockPIN 3
#define SR_dataPIN 4

bool stopwatchPauseFlag = 1;
bool timerPauseFlag = 1;
uint8_t MODE = 2;
uint8_t timeMODE = 0;
String number = "";
Time stopwatchCurrentTime, timeALLZero, timerCurrentTime, timerResetTime;
uint8_t stopwatchMilli;

uint8_t timeOffset = 3;
int timeOffsetMicro = 603;

//initialization
SoftwareSerial Bluetooth(5, 6);
String inputString;

DS3231 rtc(A0, A1);


//String EXCEPTIONS = "";
bool DecimalPointToPins[9][4]{ { 1, 1, 1, 0 },  //the map between the 4 controlling pins and the LP pin of the nixie tube.
                               { 0, 1, 1, 0 },
                               { 1, 0, 1, 0 },
                               { 0, 0, 1, 0 },
                               { 1, 1, 0, 0 },
                               { 0, 1, 0, 0 },
                               { 1, 0, 0, 0 },
                               { 0, 0, 0, 0 },
                               { 1, 1, 1, 1 } };

uint8_t DigitsToPins[11]{ 0x8, 0x0, 0xC, 0x4, 0xB, 0x3, 0x9, 0x1, 0xA, 0x2, 0xF };  //the map between the output of shift registers and the output of  4 bits decoders. 0xF represents no output!!!



/*
  given a string of 8 digits number/float,
  check if the number/float is valid, if not, try to make it valid.
  If it fails to make it valid, than return false.
*/
bool isNumberValid(String& num) {
  if (num.length() > 8) {
    //EXCEPTIONS = "[Failed]: False digits length";
    return false;
  }
  int countofDecimalPoints = 0;
  for (int i = 0; i < num.length(); ++i) {
    if (num[i] == '.') countofDecimalPoints++;
    if (num[i] == '.' && countofDecimalPoints > 1) num[i] = '0';
    if (num[i] < 48 && num[i] != 46 || num[i] > 57) {
      //EXCEPTIONS = "[Failed]: Found unsupported characters";
      return false;
    }
  }
  if (countofDecimalPoints > 1) {
    //EXCEPTIONS = "[Warning]: change extra decimal points to 0";
  }

  return true;
}
/*
  given the index of the nixie tube,
  light up the nixie tube with a decimal point.
  !!!This function could still show the decimal point while the nixie tube is showing a digit!!!
*/
void showDecimalPoint(uint8_t index = -1) {
  if (index == -1) {
    digitalWrite(dotA, DecimalPointToPins[8][0]);
    digitalWrite(dotB, DecimalPointToPins[8][1]);
    digitalWrite(dotC, DecimalPointToPins[8][2]);
    digitalWrite(dotD, DecimalPointToPins[8][3]);
  } else {
    digitalWrite(dotA, DecimalPointToPins[index][0]);
    digitalWrite(dotB, DecimalPointToPins[index][1]);
    digitalWrite(dotC, DecimalPointToPins[index][2]);
    digitalWrite(dotD, DecimalPointToPins[index][3]);
  }
  return;
}

/*
  given the an array of 0-9 digits, 10 represents no output,
  output all 8 digits to the nixie tubes in the order from left to right.
*/
void NumberArrayToScreen(uint8_t outputDigits[8]) {
  digitalWrite(SR_latchPIN, LOW);
  //using shift register
  for (uint8_t i = 0; i < 4; ++i) {
    int tmp = (DigitsToPins[outputDigits[i * 2]] << 4) + DigitsToPins[outputDigits[i * 2 + 1]];
    shiftOut(SR_dataPIN, SR_clockPIN, MSBFIRST, tmp);
  }
  digitalWrite(SR_latchPIN, HIGH);
  return;
}
/*
  set the timer to the given string
*/
Time stringToTime(String str) {
  Time timer;

  timer.year = 2000 + str.substring(0, 2).toInt();
  timer.mon = str.substring(2, 4).toInt();
  timer.date = str.substring(4, 6).toInt();

  timer.hour = str.substring(6, 8).toInt();
  timer.min = str.substring(8, 10).toInt();
  timer.sec = str.substring(10, 12).toInt();

  return timer;
}

/*
  add time or minue time
*/
void updateCurrentTime(Time& timer, int addnum) {

  timer.sec += addnum;
  if (addnum < 0) {
    if (timer.sec == 255) {  //since time->sec is a uint8_t, it can only represent 0-255. if we minus one when it is zero, it will become 255.
      timer.sec = 59;
      timer.min -= 1;
      if (timer.min == 255) {  //same theory (^-^)
        timer.min = 59;
        timer.hour -= 1;
      }
    }
  }

  if (addnum > 0) {
    timer.min += timer.sec / 60;
    timer.sec = timer.sec % 60;

    timer.hour += timer.min / 60;
    timer.min = timer.min % 60;

    timer.date += timer.hour / 24;
    timer.hour = timer.hour % 24;
  }
}
/*
  given the left number, the index of decimal point and the right number,
  ouput an integer or a float to the nixie tubes.
  Both numbers are right aligned.
*/
void outputNumber(long wholeNumberPart, int decimalPointIndex = -1, long decimalPart = -1) {
  uint8_t outputDigits[8] = { 0 };
  if (decimalPointIndex == -1) {
    //Do not show decimal point
    showDecimalPoint(-1);

    //seperate each digits into an array
    for (int i = 0; i < 8; ++i) {
      if (wholeNumberPart != 0) {
        outputDigits[7 - i] = wholeNumberPart % 10;  //save each digits into the array in the order from left to right
        wholeNumberPart /= 10;
      }
    }
  } else {
    //show decimal point at selected location
    showDecimalPoint(decimalPointIndex);

    outputDigits[decimalPointIndex] = 10;
    for (int i = 0; i < 8; ++i) {
      if (wholeNumberPart != 0) {
        outputDigits[decimalPointIndex - i - 1] = wholeNumberPart % 10;  //save each digits into the array in the order from left to right
        wholeNumberPart /= 10;
      }
      if (decimalPart != 0) {
        outputDigits[7 - i] = decimalPart % 10;  //save each digits into the array in the order from left to right
        decimalPart /= 10;
      }
    }
  }
  NumberArrayToScreen(outputDigits);
  return;
}

/*
  given 3 two digits number, 
  ouput them to the nixie tubes with a space between each two digits number.
*/
void outputTime(uint8_t aa, uint8_t bb, uint8_t cc) {
  aa %= 100;
  bb %= 100;
  cc %= 100;
  showDecimalPoint(-1);
  uint8_t outputDigits[8]{ aa / 10, aa % 10, 10, bb / 10, bb % 10, 10, cc / 10, cc % 10 };
  NumberArrayToScreen(outputDigits);
  return;
}

void chaos(uint8_t rest) {
  outputNumber(12345678);
  delay(rest);
  outputNumber(23456781);
  delay(rest);
  outputNumber(34567812);
  delay(rest);
  outputNumber(45678123);
  delay(rest);
  outputNumber(56781234);
  delay(rest);
  outputNumber(67812345);
  delay(rest);
  outputNumber(78123456);
  delay(rest);
  outputNumber(81234567);
  delay(rest);
}

/*
  if the bluetooth module recieve any string, it temporarily stores it in input_string.
  If no data is recieved in 2 miliseconds, the later data will be considered as a new input string.
*/
void getInputFromBluetooth() {
  String temp_string = "";                  // store temp input data
  while (Bluetooth.available()) {           // if there is data that can be read，then loop
    temp_string += (char)Bluetooth.read();  // join the new data into temp string
    delay(2);
  }
  inputString = temp_string;  // save to global variable.
}

/*
  check if a string has a decimal point. 
  If there is, return the index of that decimal point.
  If not, return -1.
*/
int isFloat(String str) {
  for (int i = 0; i < str.length(); i++) {
    if (str[i] == '.') return i;
  }
  return -1;
}

/*
  Since running the programm also cost some time, 
  we need to subtract a certain amount of time when we need to delay one second.

  real delay time = 1000ms - timeOffset (ms) + timeOffsetMicro (us)
*/
void delayOneSecondWithOffset() {
  if (millis() % 10000 <= 1000) {
    delay(200 - timeOffset);
    delayMicroseconds(timeOffsetMicro);
    chaos(100);
  } else
    delay(1000 - timeOffset);
  delayMicroseconds(timeOffsetMicro);
}

const String INSTR_MODE = "[MODE]";
const String INSTR_NUMBER = "[NUMBER]";
const String INSTR_SYNCTIME = "[SYNCTIME]";
const String INSTR_TIMEMODE = "[TIMEMODE]";
const String INSTR_STOPWATCH = "[STOPWATCH]";
const String INSTR_TIMER = "[TIMER]";

const uint8_t INSTR_MODE_LENGTH = INSTR_MODE.length();
const uint8_t INSTR_NUMBER_LENGTH = INSTR_NUMBER.length();
const uint8_t INSTR_SYNCTIME_LENGTH = INSTR_SYNCTIME.length();
const uint8_t INSTR_TIMEMODE_LENGTH = INSTR_TIMEMODE.length();
const uint8_t INSTR_STOPWATCH_LENGTH = INSTR_STOPWATCH.length();
const uint8_t INSTR_TIMER_LENGTH = INSTR_TIMER.length();

void setup() {


  timerCurrentTime.hour = 1;  //default stopWatch time

  rtc.begin();  //rtc clock begin

  Serial.begin(9600);
  Bluetooth.begin(9600);

  pinMode(SR_latchPIN, OUTPUT);
  pinMode(SR_clockPIN, OUTPUT);
  pinMode(SR_dataPIN, OUTPUT);

  pinMode(dotA, OUTPUT);
  pinMode(dotB, OUTPUT);
  pinMode(dotC, OUTPUT);
  pinMode(dotD, OUTPUT);
}

void loop() {
  unsigned long start=micros();
  getInputFromBluetooth();

  //if (inputString != "") Serial.println("[Info]: Input string:" + String(inputString));

  //Change Mode
  if (inputString.substring(0, INSTR_MODE_LENGTH) == INSTR_MODE) {
    MODE = inputString.substring(INSTR_MODE_LENGTH).toInt();

    //Bluetooth.println("Changed to MODE" + String(MODE));  //debug output
    //Serial.println("Changed to MODE" + String(MODE));     //debug output
  }
  //change the number of mode 1
  else if (inputString.substring(0, INSTR_NUMBER_LENGTH) == INSTR_NUMBER) {
    number = inputString.substring(INSTR_NUMBER_LENGTH);
    //Bluetooth.println("Changed number to: " + number);  //debug output
  }


  else if (inputString.substring(0, INSTR_SYNCTIME_LENGTH) == INSTR_SYNCTIME) {
    String parameter = inputString.substring(INSTR_SYNCTIME_LENGTH);
    Time timeNeedToBeSet = stringToTime(parameter);

    rtc.setDate(timeNeedToBeSet.date, timeNeedToBeSet.mon, timeNeedToBeSet.year);  //day,month,year
    rtc.setTime(timeNeedToBeSet.hour, timeNeedToBeSet.min, timeNeedToBeSet.sec);

    //Bluetooth.println("set current time: " + inputString.substring(INSTR_SYNCTIME_LENGTH));  //debug output
  }

  //change the time mode of mode 2
  else if (inputString.substring(0, INSTR_TIMEMODE_LENGTH) == INSTR_TIMEMODE) {
    timeMODE = inputString.substring(INSTR_TIMEMODE_LENGTH).toInt();
    //Bluetooth.println("change time mode to" + inputString.substring(INSTR_TIMEMODE_LENGTH));  //debug output
  }

  //configuration of mode 3
  else if (inputString.substring(0, INSTR_STOPWATCH_LENGTH) == INSTR_STOPWATCH) {
    if (inputString.substring(INSTR_STOPWATCH_LENGTH) == "RESET") {
      stopwatchCurrentTime = timeALLZero;
      stopwatchPauseFlag = 1;
      stopwatchMilli=0;
      //Bluetooth.println("RESET TIME TO 0");  //debug output
    } else if (inputString.substring(INSTR_STOPWATCH_LENGTH) == "PAUSE") {
      stopwatchPauseFlag = 1;
      //Bluetooth.println("Paused");  //debug output
    } else if (inputString.substring(INSTR_STOPWATCH_LENGTH) == "CONTINUE") {
      stopwatchPauseFlag = 0;
      //Bluetooth.println("Continue");  //debug output
    }
  }
  //configuration of mode 4
  else if (inputString.substring(0, INSTR_TIMER_LENGTH) == INSTR_TIMER) {
    if (inputString.substring(INSTR_TIMER_LENGTH) == "CONTINUE") {
      timerPauseFlag = 0;
      //Bluetooth.println("Continue");  //debug output
    } else if (inputString.substring(INSTR_TIMER_LENGTH) == "PAUSE") {
      timerPauseFlag = 1;
      //Bluetooth.println("Paused");  //debug output
    } else if (inputString.substring(INSTR_TIMER_LENGTH) == "RESET") {
      timerPauseFlag = 1;
      timerCurrentTime = timerResetTime;
      //Bluetooth.println("reset");  //debug output
    } else {
      String parameter = inputString.substring(INSTR_TIMER_LENGTH);

      timerResetTime = stringToTime(parameter);
      timerCurrentTime = timerResetTime;

      timerPauseFlag = 1;
      //Bluetooth.println("set initial time: " + inputString.substring(INSTR_TIMER_LENGTH));  //debug output
    }
  }



  switch (MODE) {
    //Reserved
    //constantly showing one number
    case 1:
      {
        //Serial.println("[info]: MODE 1");
        int index = isFloat(number);
        if (index != -1) {
          outputNumber(number.substring(0, index).toInt(), index, number.substring(index + 1).toInt());
        } else {
          outputNumber(number.toInt());
        }
        delayOneSecondWithOffset();
        break;
      }
    //showing time
    case 2:
      {
        
        Time now = rtc.getTime();
        //Serial.println("[info]: MODE 2");

        if (timeMODE == 0) {
          outputTime(now.hour, now.min, now.sec);
        }
        if (timeMODE == 1) {
          outputTime(now.hour % 12, now.min, now.sec);
        }
        if (timeMODE == 2) {
          outputTime(now.year%100, now.mon, now.date);
        }
        //unsigned long end=micros();

        //Serial.println(end-start);
        delayOneSecondWithOffset();
        break;
      }
    //timer
    case 3:
      {
        //Serial.println("[info]: MODE 3");
        outputTime(stopwatchCurrentTime.min, stopwatchCurrentTime.sec, stopwatchMilli);
        if (stopwatchPauseFlag == 0 && stopwatchMilli==99) {
          updateCurrentTime(stopwatchCurrentTime, 1);
        }
        if (stopwatchPauseFlag == 0) {
          stopwatchMilli+=1;
          stopwatchMilli%=100;
        }
        //unsigned long end=micros();

        //Serial.println(end-start);

        delay(9);
        delayMicroseconds(136);
        break;
      }
    //stopwatch
    case 4:
      {
        //Serial.println("[info]: MODE 4");
        if (timerCurrentTime.hour == 0 && timerCurrentTime.min == 0 && timerCurrentTime.sec == 0) {
          timerPauseFlag = 1;
        }
        outputTime(timerCurrentTime.hour, timerCurrentTime.min, timerCurrentTime.sec);
        if (timerPauseFlag == 0) {
          updateCurrentTime(timerCurrentTime, -1);
        }

        delayOneSecondWithOffset();
        break;
      }
    //Undefined
    default:
      {
        //Serial.println("[info]: MODE UNDEFINED");
        outputNumber(99999999);

        delayOneSecondWithOffset();
        break;
      }
  }
}
