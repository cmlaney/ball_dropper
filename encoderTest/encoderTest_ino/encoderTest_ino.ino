
#define FORWARD 1
#define REVERSE 0

int direction = 0;
int stateCounter = 0;

long counter = 0;
int fault = -1;

void setup() {
  DDRD &= !((1<<DDD2)|(1<<DDD3));
  attachInterrupt(0, encodeA, CHANGE);
  attachInterrupt(1, encodeB, CHANGE);
  Serial.begin(9600);
}

void loop() {
  if(direction==FORWARD) {Serial.println("Direction: Forward");}
  else {Serial.println("Direction: Reverse");}
  Serial.print("Counter: ");
  Serial.println(counter);
  Serial.print("Faults: ");
  Serial.println(fault);
  Serial.println("");
  delay(500);
}

void encodeA() {
  if(getStateA() & !getStateB()) {direction = FORWARD;}
  
  if(direction == FORWARD) {
    if(getStateA() & !getStateB()) {
      if(stateCounter != 3) {fault++; stateCounter = 0;}
      else{stateCounter = ++stateCounter % 4;}
    }
  
    if(!getStateA() & getStateB()) {
      if(stateCounter != 1) {fault++; stateCounter = 2;}
      else{stateCounter = ++stateCounter % 4;}
    }
    
    counter++;
  }
  
  if(direction == REVERSE) {
    if(getStateA() & getStateB()) {
      if(stateCounter != 0) {fault++; stateCounter = 1;}
      else{stateCounter = ++stateCounter % 4;}
    }
  
    if(!getStateA() & !getStateB()) {
      if(stateCounter != 2) {fault++; stateCounter = 3;}
      else{stateCounter = ++stateCounter % 4;}
    }
  }
  
  counter--;
}

void encodeB() {
  if(getStateB() & !getStateA()) {direction = REVERSE;}
  
  if(direction == FORWARD) {
    if(getStateA() & getStateB()) {
      if(stateCounter != 0) {fault++; stateCounter = 1;}
      else{stateCounter = ++stateCounter % 4;}
    }
  
    if(!getStateA() & !getStateB()) {
      if(stateCounter != 2) {fault++; stateCounter = 3;}
      else{stateCounter = ++stateCounter % 4;}
    }
    
    counter++;
  }
  
  if(direction == REVERSE) {
    if(!getStateA() & getStateB()) {
      if(stateCounter != 3) {fault++; stateCounter = 0;}
      else{stateCounter = ++stateCounter % 4;}
    }
  
    if(getStateA() & !getStateB()) {
      if(stateCounter != 1) {fault++; stateCounter = 2;}
      else{stateCounter = ++stateCounter % 4;}
    }
  }
  
  counter--;
}

void getStateA() {
  if((PIND >> PIND2) & 0x01 == 1) {
    return true;
  } else {
    return false;
  }
}

void getStateB() {
  if((PIND >> PIND3) & 0x01 == 1) {
    return true;
  } else {
    return false;
  }
}
