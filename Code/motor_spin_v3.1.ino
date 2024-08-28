#include <TM1637Display.h>
#include "Countimer.h"

// NOTE: This timer will drift by 1s per minute counted (roughly)
// Do not use for any serious measurement or timkeeping...

const int DUTYCYCLE = 5;
int upperBound = 16; // this will work out to be upperBound - 1
bool timedCycle;
int duration;
volatile bool halt = true;
volatile bool endless = false;
bool motorCW;


uint8_t rotCounter = 0;
int rotCurrentStateCLK;
int rotLastStateCLK;
String rotCurrentDir = "";

const uint8_t done[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_C | SEG_E | SEG_G,                           // n
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
};

const uint8_t inf[] = {
  false,
  SEG_B | SEG_C,                                   // I
  SEG_C | SEG_E | SEG_G,                           // n
  SEG_A | SEG_E | SEG_F | SEG_G,                   // f
  
};

const uint8_t hold[]= {
  SEG_E | SEG_C | SEG_G | SEG_D,
  SEG_E | SEG_C | SEG_G | SEG_D,
  SEG_E | SEG_C | SEG_G | SEG_D,
  SEG_E | SEG_C | SEG_G | SEG_D,
};


const uint8_t boot[]{
  SEG_F | SEG_C | SEG_D | SEG_E | SEG_G,   // b
  SEG_C | SEG_D | SEG_E | SEG_G,           // o
  SEG_C | SEG_D | SEG_E | SEG_G,           // o
  SEG_E | SEG_F | SEG_G,                   // t

};


// Pins
const byte halt_pin = 2;
const byte inf_pin = 3;
const byte displayClk_pin = 11;
const byte displayData_pin = 12;
const byte coilA_pin = 8;
const byte coilB_pin = 7;
const byte rotClk_pin =5;
const byte rotDT_pin = 6;


TM1637Display display = TM1637Display(displayClk_pin, displayData_pin);
Countimer timer;


void setup() {
  Serial.begin(9600);

  // put your setup code here, to run once:
  pinMode(halt_pin, INPUT_PULLUP);
  pinMode(inf_pin, INPUT_PULLUP);
  pinMode(displayClk_pin, OUTPUT);
  pinMode(displayData_pin, OUTPUT);
  pinMode(coilA_pin, OUTPUT);
  pinMode(coilB_pin, OUTPUT);
  pinMode(rotClk_pin, INPUT);
  pinMode(rotDT_pin, INPUT);

  digitalWrite(coilA_pin, LOW);
  digitalWrite(coilB_pin, LOW);

  rotLastStateCLK = digitalRead(rotClk_pin);

  
  attachInterrupt(digitalPinToInterrupt(halt_pin), sethalt, RISING); // change means when a thing happens -> make sure to apply some sort of filtering, schmitt trigger?
 
  display.setBrightness(0);
  display.clear();
  display.setSegments(boot);
  
}

void loop() {

  
    while((halt)){
    
    rotCurrentStateCLK = digitalRead(rotClk_pin);


    // If last and current state of CLK are different, then pulse occurred
    // React to only 1 state change to avoid double count
    if (rotCurrentStateCLK != rotLastStateCLK  && rotCurrentStateCLK == 1){

      

      // If the DT state is different than the CLK state then
      // the encoder is rotating CCW so decrement
      if (digitalRead(rotDT_pin) != rotCurrentStateCLK) {
        rotCounter ++;
        rotCurrentDir ="CW";
      } else {
        // Encoder is rotating CW so increment
        rotCounter --;
        rotCurrentDir ="CCW";
      }
      rotCounter = constrain(rotCounter,1,upperBound);

      duration = rotCounter;

      if(rotCounter==upperBound){
        display.setSegments(inf);
      }
      else {
        display.showNumberDec(duration);
      }
      

      Serial.print("Direction: ");
      Serial.print(rotCurrentDir);
      Serial.print(" | Counter: ");
      Serial.println(rotCounter);

      

    }

    // Remember last CLK state
    rotLastStateCLK = rotCurrentStateCLK;
    delay(1); 
  }
  if(!halt){
    timer.run();
    timer.start();
  }

  

  

}

void sethalt(){

  display.clear();
  timer.stop();

  if(!halt){
    timer.stop();
    stopMotor();
    

    Serial.println("Time input");
    if (duration == upperBound){
      display.setSegments(inf);
    }
    else{
      display.showNumberDec(duration);
    }

  }
  else if ((halt) && (duration < upperBound)){
    Serial.println("Countdown");
    timer.setCounter(0,duration,0, timer.COUNT_DOWN, onComplete);
    timer.setInterval(refreshClock, 1000);
  }
  else if ((halt) && (duration == upperBound)){
    Serial.println("Infinite Mode");
    timer.setCounter(24,0,0, timer.COUNT_DOWN, onComplete);
    timer.setInterval(refreshClockInfinite, 1000);
  }

  halt = !halt;
}

void onComplete(){
  Serial.println("times done!");
  halt = true;
  timer.stop();
  stopMotor();

  if(!(duration == 0)){
    display.setSegments(done);
  }
  else{
    display.setSegments(boot);
  }
  
}

bool runMotor(int time){
  Serial.print("Clockwise: ");
  Serial.println((String) motorCW);

  Serial.print("time var: ");
  Serial.println(time);

  Serial.println("time in Seconds ");
  Serial.println(timer.getCurrentSeconds());

  Serial.println("time in Min ");
  Serial.println(timer.getCurrentMinutes());



  if(((time % DUTYCYCLE) == 0) && (motorCW)){
    // slowdown motor to stop
    digitalWrite(coilA_pin, LOW);
    digitalWrite(coilB_pin, LOW);
    delay(100);
    // set new direction
    digitalWrite(coilA_pin, LOW);
    digitalWrite(coilB_pin, HIGH);
    motorCW = !motorCW; // reverse the direction for next time
  }
  else if(((time % DUTYCYCLE) == 0) && (!motorCW)){
    // slowdown motor to stop
    digitalWrite(coilA_pin, LOW);
    digitalWrite(coilB_pin, LOW);
    delay(100);
    // set new direction
    digitalWrite(coilA_pin, HIGH);
    digitalWrite(coilB_pin, LOW);
     // reverse the direction for next time
    motorCW = !motorCW; 
  }

  return motorCW;
}

void refreshClock(){

  motorCW = runMotor(timer.getCurrentSeconds());
  int calculatedTime =((timer.getCurrentMinutes()*100))+timer.getCurrentSeconds();
  Serial.print("time ");
  Serial.println(calculatedTime);
  display.showNumberDecEx(calculatedTime,0b11100000,true);

}

void refreshClockInfinite(){

  motorCW = runMotor(timer.getCurrentSeconds());
  display.setSegments(hold);

}

void stopMotor(){
  digitalWrite(coilA_pin, LOW);
  digitalWrite(coilB_pin, LOW);
}



