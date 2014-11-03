#include <avr/io.h>
#include <PinChangeInt.h>
#include <PinChangeIntConfig.h>

#define encoder0PinA A3
#define encoder0PinB A4

//#define MOTPIN1 8
//#define MOTPIN2 9

#define ROTATEMOTORLIMIT 12
#define ROTATESPEEDCONTROL 11
#define ROTATESPEED 0
#define ROTATEMOTORFORWARDCONTROLPIN 7
#define ROTATEMOTORREVERSECONTROLPIN 6

#define FORWARD 1
#define REVERSE 0

volatile int counter = 0;
volatile int direction = 0;
volatile int state = 0;
volatile int prevState = 0;
volatile int currentState = 0;
volatile int fault = 0;
volatile int prevFault = 0;

long startTime, stopTime, timeDelay = 500;

void setup() {
  pinMode(encoder0PinA, INPUT);
  pinMode(encoder0PinB, INPUT);
  //pinMode(MOTPIN1, OUTPUT);
  //pinMode(MOTPIN2, OUTPUT);
  
  //attachInterrupt(0, encode, CHANGE);
  //attachInterrupt(1, encode, CHANGE);
  PCintPort::attachInterrupt(encoder0PinA, encode, CHANGE);
  PCintPort::attachInterrupt(encoder0PinB, encode, CHANGE);
  //digitalWrite(MOTPIN1, LOW);
  //digitalWrite(MOTPIN2, LOW);
  
  pinMode(ROTATEMOTORFORWARDCONTROLPIN, OUTPUT);
  pinMode(ROTATEMOTORREVERSECONTROLPIN, OUTPUT);
  
  digitalWrite(ROTATEMOTORFORWARDCONTROLPIN, LOW);
  digitalWrite(ROTATEMOTORREVERSECONTROLPIN, LOW);
  
 Serial.begin(9600); 
 delay(1000);
}

void loop() {
  analogWrite(ROTATESPEEDCONTROL, ROTATESPEED);
  digitalWrite(ROTATEMOTORFORWARDCONTROLPIN, HIGH);
  //digitalWrite(MOTPIN1, HIGH);
  //delay(1000);
  //digitalWrite(MOTPIN1, LOW);
  //if(fault != prevFault){printInfo(); prevFault = fault;}
  //delay(500);
  //digitalWrite(MOTPIN2, HIGH);
  //delay(1000);
  //digitalWrite(MOTPIN2, LOW);
  printInfo();
  delay(500);
}

void printInfo() {
  Serial.print("Counter: ");
  Serial.println(counter);
  //if(direction==FORWARD) {Serial.println("Direction = xforward");}
  //else {Serial.println("Direction = reverse");}
  Serial.print("Faults: ");
  Serial.println(fault);
  Serial.println("");
  //fault = 0;
  //counter = 0;
}

void encode() {
  int A = (PIND&(1<<PIND2))>>PIND2;
  int B = (PIND&(1<<PIND3))>>PIND3;
  currentState = getState(digitalRead(encoder0PinA), digitalRead(encoder0PinB));
  
  if(((prevState +1)%4) == currentState) {
    counter++;
  } else if (((prevState -1 +4)%4) == currentState) {
    counter--;
  } else {
    fault++;
  }
  prevState = currentState;
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

void encodeA() {
  if(readEncodePinA() && !readEncodePinB()) {
    direction = FORWARD;
  }
  
  if(direction == FORWARD) {
    if(readEncodePinA() && !readEncodePinB()) {
      if(state != 3) {state = 0; fault++;}
      else {state = ++state % 4;}
    } else {
      if(state != 1) {state = 2; fault++;}
      else {state = ++state % 4;} 
    }
    
    counter++;
  } else {
    if(readEncodePinA() && readEncodePinB()) {
      if(state != 0) {state = 1; fault++;}
      else {state = ++state % 4;}
    } else {
      if(state != 2) {state = 3; fault++;}
      else {state = ++state % 4;} 
    }
    
    counter--;
  }
}

void encodeB() {
  if(!readEncodePinA() && readEncodePinB()) {
    direction = REVERSE;
  }
  
  if(direction == FORWARD) {
    if(readEncodePinA() && readEncodePinB()) {
      if(state != 0) {state = 1; fault++;}
      else {state = ++state % 4;}
    } else {
      if(state != 2) {state = 3; fault++;}
      else {state = ++state % 4;} 
    }
    
    counter++;
  } else {
    if(!readEncodePinA() && readEncodePinB()) {
      if(state != 3) {state = 0; fault++;}
      else {state = ++state % 4;}
    } else {
      if(state != 1) {state = 2; fault++;}
      else {state = ++state % 4;} 
    }
    
    counter--;
  }
}

boolean readEncodePinA() {
  if((PIND&(1<<PIND2))>>PIND2) {
    return true;
  } else {
    return false;
  }
}

boolean readEncodePinB() {
  if((PIND&(1<<PIND3))>>PIND3) {
    return true;
  } else {
    return false;
  }
}




