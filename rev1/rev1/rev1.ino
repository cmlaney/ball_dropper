
#include <avr/io.h>

#define FORWARD 1
#define BACKWARD 0

#define DRIVEMOTORLIMIT 10
#define DRIVEMOTORFORWARDCONTROLPIN 4
#define DRIVEMOTORREVERSECONTROLPIN 5

#define ROTATEMOTORLIMIT 12
#define ROTATEMOTORFORWARDCONTROLPIN 7
#define ROTATEMOTORREVERSECONTROLPIN 6
#define QUARTERROTATION 150
#define ROTATEENCODE1 A3
#define ROTATEENCODE2 A4

#define SYRINGEMOTORFORWARDCONTROLPIN 8
#define SYRINGEMOTORREVERSECONTROLPIN 9
#define INJECTIONDELAY 1250
#define SYRINGEENCODE1 2
#define SYRINGEENCODE2 3

#define DRIVEMOTORLED A0
#define ROTATEMOTORLED A1
#define SYRINGEMOTORLED A2

int dropCount = 0;
boolean abortDrop = false;

boolean runOnce = false;
boolean runOnceTest = true;
boolean abortFlag = false;

volatile int syringeAbsolute = 0;
volatile int counterSyringe = 0;
volatile int prevStateSyringe = 0;
volatile int currentStateSyringe = 0;
volatile int faultSyringe = 0;

volatile int counterWheel = 0;
volatile int prevStateWheel = 0;
volatile int currentStateWheel = 0;
volatile int faultWheel = 0;
volatile int errorWheel = 0;
volatile int rotateSpeed = 100;
volatile int targetWheel = 130;
volatile int pFactor = 0.2;

void setup() {
  //setup xbee
  Serial.begin(57600);
  
  //setup motor controller
  pinMode(DRIVEMOTORLIMIT, INPUT);
  pinMode(ROTATEMOTORLIMIT, INPUT);
  pinMode(DRIVEMOTORFORWARDCONTROLPIN, OUTPUT);
  pinMode(DRIVEMOTORREVERSECONTROLPIN, OUTPUT);
  pinMode(ROTATEMOTORFORWARDCONTROLPIN, OUTPUT);
  pinMode(ROTATEMOTORREVERSECONTROLPIN, OUTPUT);
  pinMode(SYRINGEMOTORFORWARDCONTROLPIN, OUTPUT);
  pinMode(SYRINGEMOTORREVERSECONTROLPIN, OUTPUT);
  pinMode(11, OUTPUT);
  
  digitalWrite(DRIVEMOTORFORWARDCONTROLPIN, LOW);
  digitalWrite(DRIVEMOTORREVERSECONTROLPIN, LOW);
  digitalWrite(ROTATEMOTORFORWARDCONTROLPIN, LOW);
  digitalWrite(ROTATEMOTORREVERSECONTROLPIN, LOW);
  digitalWrite(SYRINGEMOTORFORWARDCONTROLPIN, LOW);
  digitalWrite(SYRINGEMOTORREVERSECONTROLPIN, LOW);
  digitalWrite(11, HIGH);
  
  //setup leds 
  pinMode(DRIVEMOTORLED, OUTPUT);
  pinMode(ROTATEMOTORLED, OUTPUT);
  pinMode(SYRINGEMOTORLED, OUTPUT);
  
  digitalWrite(DRIVEMOTORLED, HIGH);
  digitalWrite(ROTATEMOTORLED, HIGH);
  digitalWrite(SYRINGEMOTORLED, HIGH);
  delay(1000);
  digitalWrite(DRIVEMOTORLED, LOW);
  digitalWrite(ROTATEMOTORLED, LOW);
  digitalWrite(SYRINGEMOTORLED, LOW);
  
  //setup limit switches
  pinMode(DRIVEMOTORLIMIT, INPUT);
  pinMode(ROTATEMOTORLIMIT, INPUT);  
  
  //setup interrupts for wheel and syringe
  //pinMode(ROTATEENCODE1, INPUT);
  //pinMode(ROTATEENCODE2, INPUT);
  pinMode(SYRINGEENCODE1, INPUT);
  pinMode(SYRINGEENCODE2, INPUT);
  
  attachInterrupt(0, encodeSyringe, CHANGE);
  attachInterrupt(1, encodeSyringe, CHANGE);
  //PCintPort::attachInterrupt(ROTATEENCODE1, encodeWheel, CHANGE);
  //PCintPort::attachInterrupt(ROTATEENCODE2, encodeWheel, CHANGE);
  
  //rotate wheel to home position
  //digitalWrite(ROTATEMOTORREVERSECONTROLPIN, HIGH);
  //delay(500);
  //while(digitalRead(ROTATEMOTORLIMIT)) {
    //delay(1);
  //}
  //digitalWrite(ROTATEMOTORREVERSECONTROLPIN, LOW);
  delay(30000);
}

void loop() {
  //on signal
  
  int i;
  if(runOnceTest) {
    rotateWheel();
    for(i=0; i<10; i++) {
      digitalWrite(ROTATEMOTORLED, HIGH);
      Serial.print("Driving ball: "); Serial.print(i+1); Serial.println("");
      digitalWrite(ROTATEMOTORLED, LOW);
      delay(500);
      digitalWrite(DRIVEMOTORLED, HIGH);
      driveBall(1);
      digitalWrite(DRIVEMOTORLED, LOW);
      Serial.print("Injecting ball: "); Serial.print(i+1); Serial.print(" Syringe absolute = "); Serial.println(syringeAbsolute);
      digitalWrite(SYRINGEMOTORLED, HIGH);
      if(!abortFlag) {
        inject();
        driveBall(0);
      }
      digitalWrite(SYRINGEMOTORLED, LOW);
      abortFlag = false;
      delay(500);
      Serial.print("Dropping ball: "); Serial.print(i+1); Serial.println("\n\n");
      rotateWheel();
      if(i==9) {
        runOnceTest = false;
        
      }
    }
    delay(500);
    Serial.println("Done.");
    syringeReverse();
    Serial.println("Syringe reversed.");
  }

}

//function to rotate ball carrier
void rotateWheel() {
  long rotateStart = millis(), rotateCurrent;
  int rotateSafetyDelay = 1000;
  
  digitalWrite(ROTATEMOTORREVERSECONTROLPIN, HIGH);
  delay(50);
  while(digitalRead(ROTATEMOTORLIMIT)) {
    //rotateCurrent = millis();
    //if(rotateCurrent-rotateStart > rotateSafetyDelay) {
      //digitalWrite(ROTATEMOTORREVERSECONTROLPIN, LOW);
      //abortDrop = true;
      //return;
    //}
    delay(1);
  }
  
  digitalWrite(ROTATEMOTORREVERSECONTROLPIN, LOW);
  
  digitalWrite(ROTATEMOTORFORWARDCONTROLPIN, HIGH);
  while(digitalRead(ROTATEMOTORLIMIT)) {
   delay(1); 
  }
  digitalWrite(ROTATEMOTORFORWARDCONTROLPIN, LOW);
}

//function to inject fluid
void inject() {
  
  long driveStart = millis(), driveCurrent;
  int driveSafetyDelay = 2000;
  
  counterSyringe = 0;
  digitalWrite(SYRINGEMOTORFORWARDCONTROLPIN, HIGH);
  while(counterSyringe < INJECTIONDELAY) {
    if(millis()-driveStart > driveSafetyDelay) {
      break;
    }
    delay(10);
  }
  digitalWrite(SYRINGEMOTORFORWARDCONTROLPIN, LOW);
}

void syringeReverse() {
  digitalWrite(SYRINGEMOTORREVERSECONTROLPIN, HIGH);
  while(syringeAbsolute > 0) {
    delay(1);
  }
  digitalWrite(SYRINGEMOTORREVERSECONTROLPIN, LOW);
}

//function to move ball forward/backward
void driveBall(int dir) {
  
  long driveStart = millis();
  int driveSafetyDelay = 3000;
  
   if(dir == FORWARD) {
     digitalWrite(DRIVEMOTORFORWARDCONTROLPIN, HIGH);
     delay(400);
     
     while(digitalRead(DRIVEMOTORLIMIT)) {
       if(millis()-driveStart > driveSafetyDelay) {
         Serial.println("Jammed");
         driveBall(0);
         abortFlag = true;
         break;
       }
       delay(10);
     }
     delay(300);
     
     digitalWrite(DRIVEMOTORFORWARDCONTROLPIN, LOW);
     
   } else if (dir == BACKWARD) {
     
     digitalWrite(DRIVEMOTORREVERSECONTROLPIN, HIGH);
     delay(600);
     
     while(digitalRead(DRIVEMOTORLIMIT)) {
       delay(1);
     }
     
     digitalWrite(DRIVEMOTORREVERSECONTROLPIN, LOW);
   }
}

void encodeWheel() {
  currentStateWheel = getState(digitalRead(ROTATEENCODE1), digitalRead(ROTATEENCODE2));
  
  if(((prevStateWheel +1)%4) == currentStateWheel) {
    counterWheel++;
  } else if (((prevStateWheel -1 +4)%4) == currentStateWheel) {
    counterWheel--;
  } else {
    faultWheel++;
  }
  prevStateWheel = currentStateWheel;
  
  errorWheel = targetWheel - counterWheel;
  if(errorWheel < 20) {errorWheel = 0;}
  rotateSpeed = errorWheel * pFactor;
}

void encodeSyringe() {
  currentStateSyringe = getState(digitalRead(SYRINGEENCODE1), digitalRead(SYRINGEENCODE2));
  
  if(((prevStateSyringe +1)%4) == currentStateSyringe) {
    counterSyringe++;
    syringeAbsolute++;
  } else if (((prevStateSyringe -1 +4)%4) == currentStateSyringe) {
    counterSyringe--;
    syringeAbsolute--;
  } else {
    faultSyringe++;
  }
  prevStateSyringe = currentStateSyringe;
}

int getState(int A, int B) {
  if(A) {
    if(B) {
      return 1;
    } else {
      return 2;
    }
  } else {
    if(B) {
      return 0;
    } else {
      return 3;
    }
  }
}

