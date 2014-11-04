#include <avr/io.h>
#include <PinChangeInt.h>

#define FORWARD 1
#define BACKWARD 0

#define DRIVEMOTORLIMIT 4
#define DRIVEMOTORCONTROLPIN 5
#define DRIVEMOTORFORWARDCONTROLPIN 6
#define DRIVEMOTORREVERSECONTROLPIN 7

#define ROTATEMOTORLIMIT 8
#define ROTATEMOTORCONTROLPIN 9
#define ROTATEMOTORFORWARDCONTROLPIN A0
#define ROTATEMOTORREVERSECONTROLPIN A1

#define SYRINGEMOTORCONTROLPIN 10
#define SYRINGEMOTORFORWARDCONTROLPIN 11
#define SYRINGEMOTORREVERSECONTROLPIN 12
#define INJECTIONDELAY 1750
#define SYRINGEENCODE1 3
#define SYRINGEENCODE2 2
#define SYRINGEABOLUTEMAX 1000

#define LED1 A3
#define LED2 A2

int dropCount = 0;
boolean abortFlagInject = false;
boolean abortFlagRetract = false;
boolean abortFlag = false;


boolean first = true;
int state = 0;
boolean driveState = true;

volatile int syringeAbsolute = 0;
volatile int counterSyringe = 0;
volatile int prevStateSyringe = 0;
volatile int currentStateSyringe = 0;
volatile int faultSyringe = 0;

void setup() {
  //setup xbee
  Serial.begin(57600);

  //setup motor controller
  pinMode(DRIVEMOTORLIMIT, INPUT);
  pinMode(ROTATEMOTORLIMIT, INPUT);

  pinMode(DRIVEMOTORCONTROLPIN, OUTPUT);
  pinMode(SYRINGEMOTORCONTROLPIN, OUTPUT);
  pinMode(ROTATEMOTORCONTROLPIN, OUTPUT);

  analogWrite(DRIVEMOTORCONTROLPIN, 180);
  analogWrite(SYRINGEMOTORCONTROLPIN, 180);
  analogWrite(ROTATEMOTORCONTROLPIN, 90);

  pinMode(DRIVEMOTORFORWARDCONTROLPIN, OUTPUT);
  pinMode(DRIVEMOTORREVERSECONTROLPIN, OUTPUT);
  pinMode(ROTATEMOTORFORWARDCONTROLPIN, OUTPUT);
  pinMode(ROTATEMOTORREVERSECONTROLPIN, OUTPUT);
  pinMode(SYRINGEMOTORFORWARDCONTROLPIN, OUTPUT);
  pinMode(SYRINGEMOTORREVERSECONTROLPIN, OUTPUT);

  digitalWrite(DRIVEMOTORFORWARDCONTROLPIN, LOW);
  digitalWrite(DRIVEMOTORREVERSECONTROLPIN, LOW);
  digitalWrite(ROTATEMOTORFORWARDCONTROLPIN, LOW);
  digitalWrite(ROTATEMOTORREVERSECONTROLPIN, LOW);
  digitalWrite(SYRINGEMOTORFORWARDCONTROLPIN, LOW);
  digitalWrite(SYRINGEMOTORREVERSECONTROLPIN, LOW);

  //setup leds 
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  digitalWrite(LED1, HIGH);
  delay(500);
  digitalWrite(LED1, LOW);

  digitalWrite(LED2, HIGH);
  delay(500);
  digitalWrite(LED2, LOW);

  //setup limit switches
  pinMode(DRIVEMOTORLIMIT, INPUT);
  pinMode(ROTATEMOTORLIMIT, INPUT);  

  //setup interrupts for syringe
  pinMode(SYRINGEENCODE1, INPUT);
  pinMode(SYRINGEENCODE2, INPUT);

  attachInterrupt(0, encodeSyringe, CHANGE);
  attachInterrupt(1, encodeSyringe, CHANGE);
}

void loop() {
  reset();
  while(Serial.available()) {
    char cmd = Serial.read();
    if(cmd == 'e') 
    {
      int val = Serial.parseInt();
      Serial.print("Dropping "); 
      Serial.print(val); 
      Serial.println(" balls."); 
      if(val > 25) {
        val = 25;
      } 
      execute(val); 
    }
    else if(cmd == 'r') 
    {
      int val = Serial.parseInt(); 
      if (val > 1) {
        int nextVal = Serial.parseInt();
        Serial.print("Rotating ");
        Serial.print(val);
        Serial.println(" times.");
        int i = 0;
        for(i = 0; i<val; i++) {
          rotateWheel(FORWARD);
          delay(nextVal);
        }
      } else if(val == 1) {
        Serial.println("Rotating forward.");
        rotateWheel(FORWARD);
      } 
      else if (val == -1) {
        Serial.println("Rotating backward.");
        rotateWheel(BACKWARD);
      } 
    }
    else if(cmd == 'd') 
    {
      int val = Serial.parseInt();
      if(val > 0) {
        Serial.println("Driving ball forward.");
        driveBall(FORWARD);
      } 
      else if (val < 0) {
        Serial.println("Driving ball backward.");
        driveBall(BACKWARD);
      }
    }
    else if(cmd == 'i') {
      int val = Serial.parseInt();
      if(val == 0) {
        Serial.println("Injecting ball.");
        inject();
      } 
      else {
        Serial.println("Repositioning syringe.");
        if(val > 5) {val = 5;}
        if(val < -5) {val = -5;}
        syringeReposition(val);
      }
    }
    else if(cmd == 'h') {
      help();
    }  
    else if(cmd == 's') {
      getState();
    }
    else {
      Serial.println("Invalid command.");
    }

    Serial.println("Done, waiting for input.\n");

  }
}

void reset() {
  digitalWrite(DRIVEMOTORFORWARDCONTROLPIN, LOW);
  digitalWrite(DRIVEMOTORREVERSECONTROLPIN, LOW);
  digitalWrite(ROTATEMOTORFORWARDCONTROLPIN, LOW);
  digitalWrite(ROTATEMOTORREVERSECONTROLPIN, LOW);
  digitalWrite(SYRINGEMOTORFORWARDCONTROLPIN, LOW);
  digitalWrite(SYRINGEMOTORREVERSECONTROLPIN, LOW);
}

void execute(int count) {
  if(!abortFlag) {
  int i;
  for(i=0; i<count; i++) 
  {
    Serial.print("Preparing to drop ball: "); 
    Serial.println(++dropCount);
    digitalWrite(LED1, HIGH);
    driveBall(FORWARD);
    digitalWrite(LED1, LOW);
    Serial.print("Injecting ball: "); 
    Serial.println(dropCount);
    digitalWrite(LED2, HIGH);
    if(!abortFlagInject) {
      inject();
    }
    digitalWrite(LED2, LOW);
    Serial.print("Retracting ball: "); 
    Serial.println(dropCount);
    driveBall(BACKWARD);
    delay(500);
    Serial.print("Dropping ball: "); 
    Serial.println(dropCount);
    if(!abortFlagRetract) {
      rotateWheel(1);
      if(abortFlag) {
        reset();
        Serial.println("Wheel jammed, land now.");
        return;
      }
    } else {
      Serial.println("IT'S ON FIRE!");
    }
  }
  } else {
    Serial.println("Wheel jammed, land now.");
    return;
  }
}

//###########################################################
//Base Functions
//###########################################################

//function to rotate ball carrier
void rotateWheel(int dir) {

  if(dir > 0) {
    digitalWrite(LED1, HIGH);
    state = 1;
    long rotateStart = millis();
    int rotateSafetyDelay = 1000;

    digitalWrite(ROTATEMOTORFORWARDCONTROLPIN, HIGH);
    delay(600);
    while(digitalRead(ROTATEMOTORLIMIT)||digitalRead(ROTATEMOTORLIMIT)) 
    {
      digitalWrite(LED1, HIGH);
      if(millis()-rotateStart > rotateSafetyDelay) 
      {
        digitalWrite(ROTATEMOTORFORWARDCONTROLPIN, LOW);
        abortFlag = true;
        error("Rotate failed.\n");
        return;
      }
      //delay(10);
    }
    digitalWrite(LED1, LOW);

    digitalWrite(ROTATEMOTORFORWARDCONTROLPIN, LOW);

    digitalWrite(ROTATEMOTORREVERSECONTROLPIN, HIGH);
    while(digitalRead(ROTATEMOTORLIMIT)) 
    {
      delay(10); 
    }
    digitalWrite(ROTATEMOTORREVERSECONTROLPIN, LOW);
    digitalWrite(LED1, LOW); 
  } 
  else {
    digitalWrite(ROTATEMOTORREVERSECONTROLPIN, HIGH);
    delay(50);
    while(digitalRead(ROTATEMOTORLIMIT)) 
    {
      delay(1);
    }
    digitalWrite(ROTATEMOTORREVERSECONTROLPIN, LOW);

    digitalWrite(ROTATEMOTORFORWARDCONTROLPIN, HIGH);
    while(digitalRead(ROTATEMOTORLIMIT)) 
    {
      delay(10); 
    }
    digitalWrite(ROTATEMOTORFORWARDCONTROLPIN, LOW);
  }

}

//function to move ball forward/backward
void driveBall(int dir) {

  long driveStart = millis();
  int driveSafetyDelay = 4000;

  if(dir == FORWARD) 
  {
    state = 2;
    digitalWrite(DRIVEMOTORFORWARDCONTROLPIN, HIGH);
    if(driveState) {
      Serial.print("Backing off");
      while(!digitalRead(DRIVEMOTORLIMIT)) {
        delay(1);        
      }
      Serial.println("Done sensing backing");
    }
    delay(10);

    while(digitalRead(DRIVEMOTORLIMIT)) 
    {
      if(millis()-driveStart > driveSafetyDelay) 
      {
        abortFlagInject = true;
        error("Ball can't drive forward.\n");
        digitalWrite(DRIVEMOTORFORWARDCONTROLPIN, LOW);
        driveState = false;
        return;
      }
      delay(10);
    }
    delay(500);

    digitalWrite(DRIVEMOTORFORWARDCONTROLPIN, LOW);
    driveState = false;
  } 
  else if (dir == BACKWARD)
  {  
    state = 4; 
    digitalWrite(DRIVEMOTORREVERSECONTROLPIN, HIGH);
    if(!driveState) {
      while(!digitalRead(DRIVEMOTORLIMIT)) {
        delay(1);
      }
      Serial.println("Done sensing backing.");
    }
    delay(10);
    while(digitalRead(DRIVEMOTORLIMIT)) 
    {
      if(millis()-driveStart > driveSafetyDelay) 
      {
        abortFlagRetract = true;
        error("Ball can't drive backward.\n");
        digitalWrite(DRIVEMOTORREVERSECONTROLPIN, LOW);
        driveState = true;
        return;
      }
      delay(10);
    }
    digitalWrite(DRIVEMOTORREVERSECONTROLPIN, LOW);
    driveState = true;
  }
  
}

//function to inject fluid
void inject() {
  state = 3;
  long injectStart = millis();
  int injectSafetyDelay = 2000;

  counterSyringe = 0;
  digitalWrite(SYRINGEMOTORFORWARDCONTROLPIN, HIGH);
  while(counterSyringe < INJECTIONDELAY) {
    if(millis()-injectStart > injectSafetyDelay) {
      abortFlag = true;
      error("Inject failed.\n");
       digitalWrite(SYRINGEMOTORFORWARDCONTROLPIN, LOW);
      return;
    }
    delay(10);
  }
  digitalWrite(SYRINGEMOTORFORWARDCONTROLPIN, LOW);
}

//###########################################################
//Other Functions
//###########################################################



void syringeReposition(int c) {
  long start = millis();
  if (c > 0) 
  {
    digitalWrite(SYRINGEMOTORFORWARDCONTROLPIN, HIGH);
    delay(c*1000);
    digitalWrite(SYRINGEMOTORFORWARDCONTROLPIN, LOW);
  } 
  else 
  {
    digitalWrite(SYRINGEMOTORREVERSECONTROLPIN, HIGH);
    delay(c*(-1000));
    digitalWrite(SYRINGEMOTORREVERSECONTROLPIN, LOW);
  }
}



//###########################################################
//Info Functions
//###########################################################

void error(String s) {
  Serial.print("ERROR: "); 
  Serial.println(s);
}


void getState() {
  Serial.print("State "); 
  Serial.println(state);
}

void help() {
  Serial.println("Commands for ball dropper:");
  Serial.println("--------------------------");
  Serial.println("e numOfBalls : Drops numOfBalls");
  Serial.println("r dir        : Rotates wheel forward if dir = 1,");
  Serial.println("               backward if dir = -1 WARNING: BACKWARD WILL BEND LIMIT SWITCH!");
  Serial.println("d dir        : Drives ball forward if dir = 1,");
  Serial.println("               backward if dir = -1");
  Serial.println("i seconds    : Injects ball if seconds = 0, repositions");
  Serial.println("               syringe forward or backward otherwise");
  Serial.println("h            : Prints command help");
  Serial.println("s            : Prints the current state of the ball dropper");
}

//###########################################################
//Encoder
//###########################################################

void encodeSyringe() {
  currentStateSyringe = getState(digitalRead(SYRINGEENCODE1), digitalRead(SYRINGEENCODE2));

  if(((prevStateSyringe +1)%4) == currentStateSyringe) {
    counterSyringe++;
    syringeAbsolute++;
  } 
  else if (((prevStateSyringe -1 +4)%4) == currentStateSyringe) {
    counterSyringe--;
    syringeAbsolute--;
  } 
  else {
    faultSyringe++;
  }
  prevStateSyringe = currentStateSyringe;
}

int getState(int A, int B) {
  if(A) {
    if(B) {
      return 1;
    } 
    else {
      return 2;
    }
  } 
  else {
    if(B) {
      return 0;
    } 
    else {
      return 3;
    }
  }
}






