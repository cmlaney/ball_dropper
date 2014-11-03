
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
#define INJECTIONDELAY 500
#define SYRINGEENCODE1 2
#define SYRINGEENCODE2 3

#define DRIVEMOTORLED A0
#define ROTATEMOTORLED A1
#define SYRINGEMOTORLED A2

int dropCount = 0;
boolean abortDrop = false;
String inString = "";

boolean first = true;

volatile int syringeAbsolute = 0;
volatile int counterSyringe = 0;
volatile int prevStateSyringe = 0;
volatile int currentStateSyringe = 0;
volatile int faultSyringe = 0;

void setup() {
  //setup xbee
  Serial.begin(57600);
  UCSR0B |= (1<<RXCIE0);
  inString.reserve(200); 
  
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
  pinMode(SYRINGEENCODE1, INPUT);
  pinMode(SYRINGEENCODE2, INPUT);
  
  attachInterrupt(0, encodeSyringe, CHANGE);
  attachInterrupt(1, encodeSyringe, CHANGE);
}

void loop() {
  //on signal
  if(inString[0] != "") {
    char command = inString[0];
    Serial.print("Command: ");
    if(inChar == 'd') {
      
      dropBall(1);
    } else if (inChar == 'e') {
      syringeReverse(0);
      Serial.println("Syringe reversed.");
    }
    else if (inChar == 'd') {
      driveBall(1);
      delay(1000);
      driveBall(0);
      Serial.println("Ball drive test.");
    }
    else if (inChar == 'f') {
      syringeReverse(1000);
      Serial.println("Syringe reversed.");
    }
    else if (inChar == 'g') {
      inject(1000);
      Serial.println("Syringe forward.");
    }
    else if (inChar == 'r') {
      rotateWheel();
      Serial.println("Rotated.");
    }
    
    inString[0] = '0';
  }
}

void dropBall(int c) {
  int i;
    if(first) {rotateWheel(); first = false;}
    for(i=0; i<c; i++) {
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
        inject(0);
        driveBall(0);
      }
      digitalWrite(SYRINGEMOTORLED, LOW);
      abortFlag = false;
      delay(500);
      Serial.print("Dropping ball: "); Serial.print(i+1); Serial.println("\n\n");
      rotateWheel();
    }
    delay(500);
    Serial.println("Done.");

}

SIGNAL(USART_RX_vect){
  char t;
  if(!(UCSR0A & (1<<RXC0))) return;
  if(QUEUE_ISFULL(uart0RxQueue)) t = DEQUEUE(uart0RxQueue);
  ENQUEUE(uart0RxQueue,UDR0);
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
void inject(int c) {
  if (c==0) {
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
  else {
      digitalWrite(SYRINGEMOTORFORWARDCONTROLPIN, HIGH);
      delay(1000);
      digitalWrite(SYRINGEMOTORFORWARDCONTROLPIN, LOW);
      Serial.println("Syringe forward.");
  }
}

void syringeReverse(int c) {
  if(c == 0){
  digitalWrite(SYRINGEMOTORREVERSECONTROLPIN, HIGH);
  while(syringeAbsolute > 0) {
    delay(1);
  }
  digitalWrite(SYRINGEMOTORREVERSECONTROLPIN, LOW);
  } else {
  digitalWrite(SYRINGEMOTORREVERSECONTROLPIN, HIGH);
  delay(c);
  digitalWrite(SYRINGEMOTORREVERSECONTROLPIN, LOW);
  }
}

//function to move ball forward/backward
void driveBall(int dir) {
  
  long driveStart = millis();
  int driveSafetyDelay = 3000;
  
   if(dir == FORWARD) {
     digitalWrite(DRIVEMOTORFORWARDCONTROLPIN, HIGH);
     delay(300);
     
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
     delay(500);
     
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

